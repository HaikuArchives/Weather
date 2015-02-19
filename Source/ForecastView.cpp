/*
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * Copyright 2014 George White
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include <Alert.h>
#include <AppKit.h>
#include <Bitmap.h>
#include <FindDirectory.h>
#include <Font.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <IconUtils.h>
#include <Language.h>
#include <LocaleRoster.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <TranslationUtils.h>
#include <Url.h>
#include <UrlProtocolRoster.h>
#include <UrlRequest.h>

#include "ForecastView.h"
#include "NetListener.h"
#include "PreferencesWindow.h"
#include "SelectionWindow.h"

const float kDraggerSize = 7;
const char* kSettingsFileName = "HaikuWeather settings";

const char* kDefaultCityName = "Menlo Park, CA";
const char* kDefaultCityId = "2449435";
const bool	kDefaultShowForecast = true;

const int32 kMaxUpdateDelay = 240;
const int32 kMaxForecastDay = 5;

extern const char* kSignature;

ForecastView::ForecastView(BRect frame, BMessage* settings)
	:
	BView(frame, "HaikuWeather", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS),
	fDownloadThread(-1),
	fReplicated(false),
	fUpdateDelay(kMaxUpdateDelay)
{

	_ApplyState(settings);
	_Init();
}

ForecastView::~ForecastView()
{
	StopReload();
}

void ForecastView::_Init() {

	_LoadBitmaps();

	BGroupLayout* root = new BGroupLayout(B_VERTICAL);
	root->SetSpacing(0);
	this->SetLayout(root);
	fView = new BGridView(2, 2);
	fLayout = fView->GridLayout();
	fLayout->SetInsets(5);
	this->AddChild(fView);

	// Icon for weather
	fConditionButton = new BButton("condition", "",
		new BMessage(kUpdateMessage));

	fConditionButton->SetIcon(fFewClouds[LARGE_ICON]);
	fLayout->AddView(fConditionButton, (int32) 0, (int32) 0);
	fConditionButton->SetFlat(true);
	fInfoView = new BGroupView(B_VERTICAL);
	BGroupLayout *infoLayout = fInfoView->GroupLayout();
	infoLayout->SetInsets(16);
	fLayout->AddView(fInfoView, (int32) 1, (int32) 0);
	
	// Description (e.g. "Mostly showers", "Cloudy", "Sunny").
	BFont bold_font(be_bold_font);
	bold_font.SetSize(18);
	fConditionView = new BStringView("description", "Loading" B_UTF8_ELLIPSIS);
	fConditionView->SetFont(&bold_font);
	infoLayout->AddView(fConditionView);

	// Numbers (e.g. temperature etc.)
	fNumberView = new BGroupView(B_HORIZONTAL);
	BGroupLayout* numberLayout = fNumberView->GroupLayout();
	infoLayout->AddView(fNumberView);
	
	BFont plain_font(be_plain_font);
	plain_font.SetSize(16);
	// Temperature (e.g. high 32 degrees C)
	fTemperatureView = new BStringView("temperature", "--");
	fTemperatureView->SetFont(&plain_font);
	numberLayout->AddView(fTemperatureView);
	
	// City
	fCityView = new BStringView("city", "--");
	fCityView->SetFont(&plain_font);
	numberLayout->AddView(fCityView);
	SetCityName(fCity);
	
	fForecastView = new BGroupView(B_HORIZONTAL);
	BGroupLayout* forecastLayout = fForecastView->GroupLayout();
	forecastLayout->SetInsets(0, 2, 0, 0);
	forecastLayout->SetSpacing(2);

	fLayout->AddView(fForecastView, (int32) 0, (int32) 1, (int32) 2);
	for (int32 i = 0; i < kMaxForecastDay; i++)
	{
		fForecastDayView[i] = new ForecastDayView(BRect(0, 0, 62, 112));
		fForecastDayView[i]->SetIcon(fFewClouds[SMALL_ICON]);
		fForecastDayView[i]->SetFahrenheit(fFahrenheit);
		forecastLayout->AddView(fForecastDayView[i]);
	}

	if (!fShowForecast) {
		fForecastView->Hide();
	}

	BRect rect(Bounds());
	rect.top = rect.bottom - kDraggerSize;
	rect.left = rect.right - kDraggerSize;
	fDragger = new BDragger(rect, this,
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);

	SetViewColor(fBackgroundColor);
	AddChild(fDragger);

	root->SetExplicitMinSize(BSize(332,228));
	root->SetExplicitMaxSize(BSize(332,228));
}

BArchivable* ForecastView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "ForecastView"))
		return NULL;

	return new ForecastView(archive);
}

ForecastView::ForecastView(BMessage* archive)
	:
	BView(archive),
	fDownloadThread(-1),
	fForcedForecast(false),
	fReplicated(true),
	fUpdateDelay(kMaxUpdateDelay)
{

	_ApplyState(archive);
	// Use _Init to rebuild the View with deep = false in Archive
	_Init();

}

status_t ForecastView::_ApplyState(BMessage* archive) {

	if (archive->FindString("city", &fCity)!= B_OK)
		fCity = kDefaultCityName;

	if (archive->FindString("cityId", &fCityId)!= B_OK)
		fCityId = kDefaultCityId;

	if (archive->FindBool("fahrenheit", &fFahrenheit) != B_OK)
		fFahrenheit = IsFahrenheitDefault();

	if (archive->FindBool("showForecast", &fShowForecast) != B_OK)
		fShowForecast = kDefaultShowForecast;

	rgb_color *color;
	ssize_t colorsize;
	status_t status;

	status = archive->FindData("backgroundColor", B_RGB_COLOR_TYPE, (const void **)&color, &colorsize);
	fBackgroundColor = (status == B_NO_ERROR) ? *color : ui_color(B_PANEL_BACKGROUND_COLOR);

	status = archive->FindData("textColor", B_RGB_COLOR_TYPE, (const void **)&color, &colorsize);
	fTextColor = (status == B_NO_ERROR) ? *color : ui_color(B_PANEL_TEXT_COLOR);

	return B_OK;
}

status_t
ForecastView::Archive(BMessage* into, bool deep) const
{
	status_t status;

	status = BView::Archive(into, false); // NO DEEP REBUILD THE VIEW
	if (status < B_OK)
		return status;

	status = into->AddString("add_on", kSignature);
	if (status < B_OK)
		return status;

	status = SaveState(into, deep);
	if (status < B_OK)
		return status;

	return B_OK;
}

status_t
ForecastView::SaveState(BMessage* into, bool deep) const
{
	status_t status;

	status = into->AddString("city", fCity);
	if (status != B_OK)
		return status;
	status = into->AddString("cityId", fCityId);
	if (status != B_OK)
		return status;
	status = into->AddBool("fahrenheit", fFahrenheit);
	if (status != B_OK)
		return status;
	status = into->AddBool("showForecast", fShowForecast);
	if (status != B_OK)
		return status;

	if (!IsDefaultColor()) {
		status = into->AddData("textColor", B_RGB_COLOR_TYPE, &fTextColor, sizeof(rgb_color));
		if (status != B_OK)
			return status;
		status = into->AddData("backgroundColor", B_RGB_COLOR_TYPE, &fBackgroundColor, sizeof(rgb_color));
		if (status != B_OK)
			return status;
	}

	return B_OK;

}

void ForecastView::AttachedToWindow() {

	if (fReplicated)
		fConditionButton->SetTarget(BMessenger(this));

	BMessenger view(this);
	BMessage autoUpdateMessage(kAutoUpdateMessage);
	fAutoUpdate = new BMessageRunner(view,  &autoUpdateMessage, (bigtime_t)fUpdateDelay * 60 * 1000 * 1000);
	view.SendMessage(new BMessage(kUpdateMessage));
	BView::AttachedToWindow();
}


void ForecastView::AllAttached() {
	SetBackgroundColor(fBackgroundColor);
	SetTextColor(fTextColor);
}

void ForecastView::MessageReceived(BMessage *msg) {
	BString tempString("");
	BString text("");

	if (msg->WasDropped()) {
		rgb_color* color = NULL;
		ssize_t size = 0;

		if (msg->FindData("RGBColor", (type_code)'RGBC', (const void**)&color,
				&size) == B_OK) {
			BPoint point;
			uint32	buttons;
			GetMouse(&point, &buttons, true);
			BMenuItem* item;
			BPopUpMenu* popup = new BPopUpMenu("PopUp", false);
			popup->AddItem(item = new BMenuItem("Background", new BMessage('BACC')));
			popup->AddItem(item = new BMenuItem("Text", new BMessage('TEXC')));
			popup->AddSeparatorItem();
			popup->AddItem(item = new BMenuItem("Default", new BMessage('DEFT')));
			item->SetEnabled(!IsDefaultColor());
			ConvertToScreen(&point);
			item = popup->Go(point);

			if (item && item->Message()->what == 'BACC')
				SetBackgroundColor(*color);

			if (item && item->Message()->what == 'TEXC')
				SetTextColor(*color);

			if (item && item->Message()->what == 'DEFT') {
				SetBackgroundColor(ui_color(B_PANEL_BACKGROUND_COLOR));
				SetTextColor(ui_color(B_PANEL_TEXT_COLOR));
			}
			return;
		}
	}

	switch (msg->what) {
	case kDataMessage:
		msg->FindInt32("temp", &fTemperature);
		msg->FindInt32("code", &fCondition);
		msg->FindString("text", &text);

		if (fFahrenheit)
			tempString << fTemperature << "℉";
		else
			tempString << static_cast<int>(floor(CEL(fTemperature))) << "℃";

		fTemperatureView->SetText(tempString);
		SetCondition(text);
		fConditionButton->SetIcon(_GetWeatherIcon(fCondition, LARGE_ICON));
		break;
	case kForecastDataMessage:
	{
		int32 forecastNum;
		BString day;
		int32 condition;
		int32 high, low;

		msg->FindInt32("forecast", &forecastNum);
		msg->FindInt32("high", &high);
		msg->FindInt32("low", &low);
		msg->FindInt32("code", &condition);
		msg->FindString("text", &text);
		msg->FindString("day", &day);

		if (forecastNum < 0 || forecastNum >= kMaxForecastDay)
			break;

		fForecastDayView[forecastNum]->SetDayLabel(day);
		fForecastDayView[forecastNum]->SetIcon(_GetWeatherIcon(condition, SMALL_ICON));
		fForecastDayView[forecastNum]->SetHighTemp(high);
		fForecastDayView[forecastNum]->SetLowTemp(low);
		fForecastDayView[forecastNum]->SetToolTip(text);
		break;
	}
	case kFailureMessage:
		SetCondition("Connection error");
		break;
	case kUpdateMessage:
		SetCondition("Loading" B_UTF8_ELLIPSIS);
	case kAutoUpdateMessage:
		Reload();
		break;
	case kShowForecastMessage:
		_ShowForecast(!fShowForecast);
		if (fShowForecast)
			Reload();
		break;
	case kUpdateCityName:{
		BString newCityName;
		if (msg->FindString("city", &newCityName) == B_OK)
			SetCityName(newCityName);
		}
		break;
	case kUpdateTTLMessage: {
		int32 ttl;
		msg->FindInt32("ttl", &ttl);
		SetUpdateDelay(ttl < kMaxUpdateDelay ? ttl : kMaxUpdateDelay);
		}
		break;
	case B_ABOUT_REQUESTED:
		{
			BAlert *alert = new BAlert("About HaikuWeather",
				"HaikuWeather (The Replicant version)", "OK");
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go();
		}	break;
	default:
		BView::MessageReceived(msg);
	}
}

void ForecastView::_LoadBitmaps() {
	_LoadIcons(fAlert, 'rGFX', "Artwork/weather_alert.hvif");
	_LoadIcons(fClearNight, 'rGFX',	"Artwork/weather_clear_night.hvif");
	_LoadIcons(fClear, 'rGFX', "Artwork/weather_clear.hvif");
	_LoadIcons(fClouds, 'rGFX',	"Artwork/weather_clouds.hvif");
	_LoadIcons(fFewClouds, 'rGFX', "Artwork/weather_few_clouds.hvif");
	_LoadIcons(fFog, 'rGFX', "Artwork/weather_fog.hvif");
	_LoadIcons(fNightFewClouds, 'rGFX', "Artwork/weather_night_few_clouds.hvif");
	_LoadIcons(fRainingScattered, 'rGFX', "Artwork/weather_raining_scattered.hvif");
	_LoadIcons(fRaining, 'rGFX', "Artwork/weather_raining.hvif");
	_LoadIcons(fShining, 'rGFX', "Artwork/weather_shining.hvif");
	_LoadIcons(fShiny, 'rGFX', "Artwork/weather_shiny.hvif");
	_LoadIcons(fSnow, 'rGFX', "Artwork/weather_snow.hvif");
	_LoadIcons(fStorm, 'rGFX', "Artwork/weather_storm.hvif");
	_LoadIcons(fThunder, 'rGFX', "Artwork/weather_thunder.hvif");
}

void ForecastView::_LoadIcons(BBitmap* bitmap[2], uint32 type, const char* name) {

	size_t dataSize;

	bitmap[0] = NULL;
	bitmap[1] = NULL;

	const void* data = NULL;

	BResources resources;
	status_t status = resources.SetToImage(&&dummy_label);
dummy_label:

	if (status == B_OK)
		data = resources.LoadResource(type, name, &dataSize);

	if (data != NULL){

		BBitmap* smallBitmap = new BBitmap(BRect(0, 0, kSizeSmallIcon - 1, kSizeSmallIcon - 1), 0,
			B_RGBA32);
		BBitmap* largeBitmap = new BBitmap(BRect(0, 0, kSizeLargeIcon - 1, kSizeLargeIcon - 1), 0,
			B_RGBA32);

		status_t status = smallBitmap->InitCheck();
		if (status == B_OK) {
			status = BIconUtils::GetVectorIcon(
				reinterpret_cast<const uint8*>(data), dataSize, smallBitmap);
		};
		status = largeBitmap->InitCheck();
		if (status == B_OK) {
			status = BIconUtils::GetVectorIcon(
				reinterpret_cast<const uint8*>(data), dataSize, largeBitmap);
		};
		bitmap[0] = smallBitmap;
		bitmap[1] = largeBitmap;
	};
}

BBitmap* ForecastView::_GetWeatherIcon(int32 condition, weatherIconSize iconSize) {

	switch (condition) {	// https://developer.yahoo.com/weather/documentation.html
		case 0:											// tornado
		case 1:											// tropical storm
		case 2:	return fAlert[iconSize];				// hurricane
		case 3:											// severe thunderstorms
		case 4:	return fStorm[iconSize];				// thunderstorms
		case 5:											// mixed rain and snow
		case 6:											// mixed rain and sleet
		case 7: return fSnow[iconSize];					// mixed snow and sleet
		case 8:											// freezing drizzle
		case 9:											// drizzle
		case 10: return fRaining[iconSize];				// freezing rain
												// 11 - 12 It isn't an error repeated
		case 11:										// showers
		case 12: return fRainingScattered[iconSize];	// showers  
		case 13:										// snow flurries
		case 14:										// light snow showers
		case 15:										// blowing snow
		case 16:										// snow
		case 17:										// hail
		case 18: return fSnow[iconSize];				// sleet
		case 19: break;									//*dust
		case 20: return fFog[iconSize];					// fog
		case 21: break;									//*haze
		case 22: break;									//*smoky
		case 23: break;									//*blustery
		case 24: break;									//*windy
		case 25: break;									//*cold
		case 26:										// cloudy
		case 27:										// mostly cloudy (night)
		case 28: return fClouds[iconSize];				// mostly cloudy (day)
		case 29: return fNightFewClouds[iconSize];		// partly cloudy (night)
		case 30: return fFewClouds[iconSize];			// partly cloudy (day)	
		case 31: return fClearNight[iconSize];			// clear (night)
		case 32: return fClear[iconSize];				// sunny
		case 33: return fClearNight[iconSize];			// fair (night)
		case 34: return fFewClouds[iconSize];			// fair (day)
		case 35: return fRainingScattered[iconSize];	// mixed rain and hail
		case 36: return fShining[iconSize];				// hot
		case 37: return fThunder[iconSize];				// isolated thunderstorms
											// 38 - 39  It isn't an error repeated
							// 39 is PM Showers, Probably a documentation error
		case 38: return fStorm[iconSize];				// scattered thunderstorms
		case 39: return fRainingScattered[iconSize];	// scattered thunderstorms
		case 40: return fRainingScattered[iconSize];	// scattered showers
		case 41:										// heavy snow
		case 42:										// scattered snow showers
		case 43: return fSnow[iconSize];				// heavy snow
		case 44: return fClouds[iconSize]; 				// partly cloudy
		case 45: return fStorm[iconSize];				// thundershowers
		case 46: return fSnow[iconSize];				// snow showers
		case 47: return fThunder[iconSize];				// isolated thundershowers
		case 3200: break;								//*not available		
		
	}
	return NULL; // Change to N/A

}

void ForecastView::SetCityId(BString cityId){
	fCityId = cityId;
}

BString ForecastView::CityId(){
	return fCityId;
}

void ForecastView::SetCityName(BString city){
	fCity = city;
    fCityView->TruncateString(&city, B_TRUNCATE_END, 150);
    if (city != fCity)
		fCityView->SetToolTip(fCity);
    else
		fCityView->SetToolTip("");
	fCityView->SetText(city);
}

BString ForecastView::CityName(){
	return fCity;
}

void ForecastView::SetUpdateDelay(int32 delay){
	if (fUpdateDelay != delay){
		fUpdateDelay = delay;
		fAutoUpdate->SetInterval((bigtime_t)fUpdateDelay * 60 * 1000 * 1000);
	}
}

int32	ForecastView::UpdateDelay(){
	return fUpdateDelay;
}

void ForecastView::SetFahrenheit(bool fahrenheit){
	BString tempString;
	fFahrenheit = fahrenheit;
	if (fFahrenheit)
		tempString << fTemperature << "℉";
	else
		tempString << static_cast<int>(floor(CEL(fTemperature))) << "℃";
		
	fTemperatureView->SetText(tempString);
	
	for (int32 i = 0; i < kMaxForecastDay; i++)
	{
		fForecastDayView[i]->SetFahrenheit(fFahrenheit);
	}
}

bool ForecastView::IsFahrenheit(){
	return fFahrenheit;
}

// TODO replace this function when the localekit one is available
bool ForecastView::IsFahrenheitDefault(){
	BMessage availableLanguages;
	if (BLocaleRoster::Default()->GetPreferredLanguages(&availableLanguages)
			== B_OK) {
		BString currentID;
		for (int i = 0; availableLanguages.FindString("language", i, &currentID)
				== B_OK; i++) {

			BLanguage currentLanguage(currentID.String());

			if (currentLanguage.CountryCode() == NULL)
				continue;
			if (strcmp(currentLanguage.CountryCode(), "US") == 0)
				return true;
			if (strcmp(currentLanguage.CountryCode(), "BS") == 0)
				return true;
			if (strcmp(currentLanguage.CountryCode(), "BZ") == 0)
				return true;
			if (strcmp(currentLanguage.CountryCode(), "PW") == 0)
				return true;
			if (strcmp(currentLanguage.CountryCode(), "KY") == 0)
				return true;
			if (strcmp(currentLanguage.CountryCode(), "GU") == 0)
				return true;
			if (strcmp(currentLanguage.CountryCode(), "PR") == 0)
				return true;
			if (strcmp(currentLanguage.CountryCode(), "VI") == 0)
				return true;
			break;
		}
	}
	return false;
}

void ForecastView::SetCondition(BString condition){
	BString conditionTruncated(condition);
	fConditionView->TruncateString(&conditionTruncated, B_TRUNCATE_END, 196);
    if (conditionTruncated != condition)
		fConditionView->SetToolTip(condition);
    else
		fConditionView->SetToolTip("");
	fConditionView->SetText(conditionTruncated);
}

void ForecastView::SetShowForecast(bool showForecast){
	_ShowForecast(showForecast);
}

bool ForecastView::ShowForecast(){
	return fShowForecast;
}

void ForecastView::Reload(bool forcedForecast) {
	StopReload();

	fForcedForecast = forcedForecast;

	fDownloadThread = spawn_thread(&_DownloadDataFunc,
		"Download Data", B_NORMAL_PRIORITY, this);
	if (fDownloadThread >= 0)
		resume_thread(fDownloadThread);
}

void ForecastView::StopReload() {
	if (fDownloadThread < 0)
		return;
	wait_for_thread(fDownloadThread, NULL);
	fDownloadThread = -1;
}

int32 ForecastView::_DownloadDataFunc(void *cookie) {
	ForecastView* forecastView = static_cast<ForecastView*>(cookie);
	forecastView->_DownloadData();
	return 0;
}

void ForecastView::_DownloadData() {

	BString urlString("https://query.yahooapis.com/v1/public/yql");
	if (fShowForecast || fForcedForecast)
		urlString << "?q=select+*+from+weather.forecast+";
	else
		urlString << "?q=select+item.condition+from+weather.forecast+";

	urlString << "where+woeid+=+" << fCityId << "&format=json";

	NetListener listener(this, WEATHER_REQUEST);
	BUrlRequest* request =
		BUrlProtocolRoster::MakeRequest(BUrl(urlString.String()),
		&listener);

	thread_id thread = request->Run();
	wait_for_thread(thread, NULL);
	delete request;
}

void ForecastView::_ShowForecast(bool show) {
	if (fShowForecast == show)
		return;
	fShowForecast = show;

	if (fShowForecast)
		fForecastView->Show();
	else
		fForecastView->Hide();
}

void ForecastView::SetTextColor(rgb_color color)
{
	fTextColor = color;
	SetHighColor(color);
	fView->SetHighColor(color);
	fInfoView->SetHighColor(color);
	fConditionButton->SetHighColor(color);
	fConditionView->SetHighColor(color);
	fTemperatureView->SetHighColor(color);
	fCityView->SetHighColor(color);
	fForecastView->SetHighColor(color);
	fNumberView->SetHighColor(color);
	for (int32 i = 0; i < kMaxForecastDay; i++) {
		fForecastDayView[i]->SetTextColor(color);
	}
	fNumberView->Invalidate();
	fInfoView->Invalidate();
	fView->Invalidate();
	fConditionButton->Invalidate();
	fConditionView->Invalidate();
	fTemperatureView->Invalidate();
	fCityView->Invalidate();
	fForecastView->Invalidate();
}
void ForecastView::SetBackgroundColor(rgb_color color)
{
	fBackgroundColor = color;
	SetViewColor(color);
	SetLowColor(color);
	fView->SetViewColor(color);
	fInfoView->SetViewColor(color);
	fConditionButton->SetLowColor(color);
	fConditionView->SetViewColor(color);
	fTemperatureView->SetViewColor(color);
	fCityView->SetViewColor(color);
	fForecastView->SetViewColor(color);
	fNumberView->SetViewColor(color);
	for (int32 i = 0; i < kMaxForecastDay; i++) {
		fForecastDayView[i]->SetViewColor(color);
		fForecastDayView[i]->Invalidate();
	}
	fNumberView->Invalidate();
	fInfoView->Invalidate();
	fView->Invalidate();
	fConditionButton->Invalidate();
	fConditionView->Invalidate();
	fTemperatureView->Invalidate();
	fCityView->Invalidate();
	fForecastView->Invalidate();
	fDragger->Invalidate();
}

bool ForecastView::IsDefaultColor() const
{
	return fBackgroundColor == ui_color(B_PANEL_BACKGROUND_COLOR)
		&& fTextColor == ui_color(B_PANEL_TEXT_COLOR);
}
