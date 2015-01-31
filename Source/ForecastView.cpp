/*
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * Copyright 2014 George White
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include <Alert.h>
#include <AppKit.h>
#include <Bitmap.h>
#include <Dragger.h>
#include <FindDirectory.h>
#include <Font.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <IconUtils.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
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
const int32	kDefaultUpdateDelay = 30;
const bool	kDefaultFahrenheit = false;
const bool	kDefaultShowForecast = false;
const BRect kDefaultForecastViewRect = BRect(150,150,0,0);
const int32 kMaxForecastDay = 5;

const bool	kReplicantEnabled = true; // NOT Completed Yet

extern const char* kSignature;

ForecastView::ForecastView(BRect frame)
	:
	BView(frame, "HaikuWeather", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS),
	fDownloadThread(-1),
	fReplicated(false)
{

	if (_LoadSettings() != B_OK) {
		fCity = kDefaultCityName;
		fCityId = kDefaultCityId;
		fUpdateDelay = kDefaultUpdateDelay;
		fFahrenheit = kDefaultFahrenheit;
		fShowForecast = kDefaultShowForecast;
		fForecastViewRect = kDefaultForecastViewRect;
	}

	_Init();
}



void ForecastView::_Init() {

	_LoadBitmaps();

	BGroupLayout* root = new BGroupLayout(B_VERTICAL);
	root->SetSpacing(0);
	this->SetLayout(root);
	fView = new BGridView(3, 3);
	fLayout = fView->GridLayout();
	fLayout->SetInsets(5);
	this->AddChild(fView);

	// Icon for weather
	fConditionButton = new BButton("condition", "",
		new BMessage(kUpdateMessage));

	fConditionButton->SetIcon(fFewClouds[LARGE_ICON]);
	fLayout->AddView(fConditionButton, (int32) 0, (int32) 0);
	
	BGroupView *infoView = new BGroupView(B_VERTICAL);
	BGroupLayout *infoLayout = infoView->GroupLayout();
	infoLayout->SetInsets(16);
	fLayout->AddView(infoView, (int32) 1, (int32) 0);
	
	// Description (e.g. "Mostly showers", "Cloudy", "Sunny").
	BFont bold_font(be_bold_font);
	bold_font.SetSize(24);
	fConditionView = new BStringView("description", "Loading" B_UTF8_ELLIPSIS);
	fConditionView->SetFont(&bold_font);
	infoLayout->AddView(fConditionView);

	// Numbers (e.g. temperature etc.)
	BGroupView* numberView = new BGroupView(B_HORIZONTAL);
	BGroupLayout* numberLayout = numberView->GroupLayout();
	infoLayout->AddView(numberView);
	
	// Temperature (e.g. high 32 degrees C)
	fTemperatureView = new BStringView("temperature", "");
	numberLayout->AddView(fTemperatureView);
	
	// City
	fCityView = new BStringView("city", fCity);
	numberLayout->AddView(fCityView);
	
	// Numbers (e.g. temperature etc.)
	fForecastView = new BGroupView(B_HORIZONTAL);
	BGroupLayout* forecastLayout = fForecastView->GroupLayout();
	forecastLayout->SetInsets(5, 0, 5, 5);
	forecastLayout->SetSpacing(2);
	this->AddChild(fForecastView);

	for (int32 i = 0; i < kMaxForecastDay; i++)
	{
		fForecastDayView[i] = new ForecastDayView(BRect(0,0,48,96));
		fForecastDayView[i]->SetIcon(fFewClouds[SMALL_ICON]);
		fForecastDayView[i]->SetFahrenheit(fFahrenheit);
		fForecastDayView[i]->SetHighTemp(fFahrenheit ? 40 : 60);
		forecastLayout->AddView(fForecastDayView[i]);
	}

	if (!fShowForecast) {
		fForecastView->Hide();
	}

	if (kReplicantEnabled) {
		BRect rect(Bounds());
		rect.top = rect.bottom - kDraggerSize;
		rect.left = rect.right - kDraggerSize;
		BDragger* dragger = new BDragger(rect, this,
			B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);

		AddChild(BLayoutBuilder::Group<>(B_HORIZONTAL)
				.AddGlue()
				.Add(dragger)
		);
	}
}


BArchivable*
ForecastView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "ForecastView"))
		return NULL;

	return new ForecastView(archive);
}

ForecastView::ForecastView(BMessage* archive)
	: BView(archive)
{
	fReplicated = true;

	fDownloadThread = -1;
	fForcedForecast = false;

	if (archive->FindString("city", &fCity)!= B_OK)
		fCity = kDefaultCityName;

	if (archive->FindString("cityId", &fCityId)!= B_OK)
		fCityId = kDefaultCityId;

	if (archive->FindInt32("updateDelay", &fUpdateDelay)!= B_OK)
		fUpdateDelay = kDefaultUpdateDelay;

	if (archive->FindBool("fahrenheit", &fFahrenheit) != B_OK)
		fFahrenheit = kDefaultFahrenheit;

	if (archive->FindBool("showForecast", &fShowForecast) != B_OK)
		fShowForecast = kDefaultShowForecast;

	if (archive->FindInt32("temperature", &fTemperature)!= B_OK)
		fTemperature = 0;

	if (archive->FindInt32("condition", &fCondition)!= B_OK)
		fCondition = 0;

	// Use _Init to rebuild the View with deep = false in Archive
	_Init();

}

void ForecastView::_BindView(){

	_LoadBitmaps();

	fConditionButton = dynamic_cast<BButton*>(FindView("condition"));

	fConditionView = dynamic_cast<BStringView*>(FindView("description"));

	fTemperatureView = dynamic_cast<BStringView*>(FindView("temperature"));

	fCityView = dynamic_cast<BStringView*>(FindView("city"));

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
	status = into->AddInt32("updateDelay", fUpdateDelay);
	if (status != B_OK)
		return status;
	status = into->AddBool("fahrenheit", fFahrenheit);
	if (status != B_OK)
		return status;
	status = into->AddBool("showForecast", fShowForecast);
	if (status != B_OK)
		return status;
	status = into->AddInt32("temperature", fTemperature);
	if (status != B_OK)
		return status;
	status = into->AddInt32("condition", fCondition);
	if (status != B_OK)
		return status;

	return B_OK;

}



void ForecastView::AttachedToWindow() {

	if (Window() != NULL && !fReplicated)
		Window()->MoveTo(fForecastViewRect.LeftTop());

	if (fReplicated)
		fConditionButton->SetTarget(BMessenger(this));

	BMessenger view(this);
	BMessage autoUpdateMessage(kAutoUpdateMessage);
	fAutoUpdate = new BMessageRunner(view,  &autoUpdateMessage, fUpdateDelay * 60 * 1000 * 1000);
	view.SendMessage(new BMessage(kUpdateMessage));
}


void	ForecastView::AllAttached() {
	/* Doesn't Work
	if (fReplicated)
		_BindView();
	*/
}

void ForecastView::MessageReceived(BMessage *msg) {
	BString tempString("");
	BString text("");

	switch (msg->what) {
	case kDataMessage:
		msg->FindInt32("temp", &fTemperature);
		msg->FindInt32("code", &fCondition);
		msg->FindString("text", &text);
		
		if (fFahrenheit)
			tempString << fTemperature << "°F";
		else
			tempString << static_cast<int>(floor(CEL(fTemperature))) << "°C";
		
		fTemperatureView->SetText(tempString);
		fConditionView->SetText(text);
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
		break;
	}
	case kFailureMessage:
		fConditionView->SetText("Connection error");
		break;
	case kUpdateMessage:
		fConditionView->SetText("Loading" B_UTF8_ELLIPSIS);
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
	case B_ABOUT_REQUESTED:
		{
			BAlert *alert = new BAlert("About HaikuWeather",
				"HaikuWeather (The Replicant version)\n\nUnder Development", "OK");
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

	const void* data;

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
		case 38:										// scattered thunderstorms
		case 39: return fStorm[iconSize];				// scattered thunderstorms
		case 40: return fRaining[iconSize];				// scattered showers
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
	fCityView->SetText(fCity);
}

BString ForecastView::CityName(){
	return fCity;
}

void ForecastView::SetUpdateDelay(int32 delay){
	if (fUpdateDelay != delay){
		fUpdateDelay = delay;
		fAutoUpdate->SetInterval(fUpdateDelay * 60 * 1000 * 1000);
	}
}

int32	ForecastView::UpdateDelay(){
	return fUpdateDelay;
}

void ForecastView::SetFahrenheit(bool fahrenheit){
	BString tempString;
	fFahrenheit = fahrenheit;
	if (fFahrenheit)
		tempString << fTemperature << "°F";
	else
		tempString << static_cast<int>(floor(CEL(fTemperature))) << "°C";
		
	fTemperatureView->SetText(tempString);
	
	for (int32 i = 0; i < kMaxForecastDay; i++)
	{
		fForecastDayView[i]->SetFahrenheit(fFahrenheit);
	}
}


bool ForecastView::IsFahrenheit(){
	return fFahrenheit;
}

void ForecastView::SetCondition(BString condition){
	fConditionView->SetText(condition);
}

void ForecastView::SetShowForecast(bool showForecast){
	_ShowForecast(showForecast);
}

bool ForecastView::ShowForecast(){
	return fShowForecast;
}

void ForecastView::Reload(bool forcedForecast = false) {
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


status_t ForecastView::_LoadSettings() {
	BPath p;
	BFile f;
	BMessage m;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &p) != B_OK)
		return B_ERROR;
	p.Append(kSettingsFileName);
	
	f.SetTo(p.Path(), B_READ_ONLY);
	if (f.InitCheck() != B_OK)
		return B_ERROR;
	
	if (m.Unflatten(&f) != B_OK)
		return B_ERROR;
	
	if (m.what != kSettingsMessage)
		return B_ERROR;
	
	if (m.FindString("fCity", &fCity) != B_OK)
		fCity = kDefaultCityName;
	if (m.FindString("fCityId", &fCityId) != B_OK)
		fCityId = kDefaultCityId;
	
	if (m.FindInt32("fUpdateDelay", &fUpdateDelay) != B_OK)
		fUpdateDelay = kDefaultUpdateDelay;

	if (m.FindBool("fFahrenheit", &fFahrenheit) != B_OK)
		fFahrenheit = kDefaultFahrenheit;

	if (m.FindBool("fShowForecast", &fShowForecast) != B_OK)
		fShowForecast = kDefaultShowForecast;
	
	if (m.FindRect("fForecastViewRect", &fForecastViewRect) != B_OK)
		fForecastViewRect = kDefaultForecastViewRect;

	return B_OK;
}


status_t ForecastView::SaveSettings() {
	BPath p;
	BFile f;
	BMessage m(kSettingsMessage);
	
	m.AddString("fCity", fCity);
	m.AddString("fCityId", fCityId);
	m.AddInt32("fUpdateDelay", fUpdateDelay);
	m.AddBool("fFahrenheit", fFahrenheit);
	m.AddBool("fShowForecast", fShowForecast);
	m.AddRect("fForecastViewRect", Window()->Frame());
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &p) != B_OK)
		return B_ERROR;
	p.Append(kSettingsFileName);
	
	f.SetTo(p.Path(), B_WRITE_ONLY | B_ERASE_FILE | B_CREATE_FILE);
	if (f.InitCheck() != B_OK)
		return B_ERROR;
	
	if (m.Flatten(&f) != B_OK)
		return B_ERROR;
	
	return B_OK;
}
