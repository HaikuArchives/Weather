/*
 * Copyright 2022 Nexus6 (Davide Alfano) <nexus6.haiku@icloud.com>
 * Copyright 2015 Adrián Arroyo Calle <adrian.arroyocalle@gmail.com>
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * Copyright 2014 George White
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include <Alert.h>
#include <AppKit.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <FindDirectory.h>
#include <Font.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <IconUtils.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <NetworkDevice.h>
#include <NetworkInterface.h>
#include <NetworkRoster.h>
#include <PopUpMenu.h>
#include <TranslationUtils.h>
#include <Url.h>
#include <UrlProtocolRoster.h>
#include <UrlRequest.h>

#include "ForecastView.h"
#include "WSOpenMeteo.h"
#include "PreferencesWindow.h"
#include "SelectionWindow.h"
#include "MainWindow.h"

const float kDraggerSize = 7;
const char* kSettingsFileName = "Weather settings";

const char* kDefaultCityName = "San Francisco";
const char* kDefaultCityId = "2487956";
const bool  kDefaultShowForecast = true;

const int32 kMaxUpdateDelay = 240;
const int32 kMaxForecastDay = 5;
const int32 kReconnectionDelay = 5;

extern const char* kSignature;

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ForecastView"

class TransparentButton : public BButton {
	public:
		TransparentButton(const char* name, const char* label, BMessage* message);
		virtual void Draw (BRect updateRect);
};


TransparentButton::TransparentButton(const char* name, const char* label, BMessage* message)
	:
	BButton(name, label, message, B_DRAW_ON_CHILDREN)
{
}


void
TransparentButton::Draw(BRect updateRect)
{
	BRect rect(Bounds());
	rgb_color background = ViewColor();
	rgb_color base = LowColor();
	rgb_color textColor = ui_color(B_CONTROL_TEXT_COLOR);

	uint32 flags = be_control_look->Flags(this);

	flags |= BControlLook::B_FLAT;

	const float kLabelMargin = 8;
	// always leave some room around the label
	rect.InsetBy(kLabelMargin, kLabelMargin);

	const BBitmap* icon = IconBitmap(
		(Value() == B_CONTROL_OFF
				? B_INACTIVE_ICON_BITMAP : B_ACTIVE_ICON_BITMAP)
			| (IsEnabled() ? 0 : B_DISABLED_ICON_BITMAP));

	be_control_look->DrawLabel(this, Label(), icon, rect, updateRect, base,
		flags, BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE), &textColor);
}


ForecastView::ForecastView(BRect frame, BMessage* settings)
	:
	BView(frame, B_TRANSLATE_SYSTEM_NAME("Weather"), B_FOLLOW_NONE,
		B_WILL_DRAW | B_FRAME_EVENTS | B_DRAW_ON_CHILDREN),
	fDownloadThread(-1),
	fReplicated(false),
	fUpdateDelay(kMaxUpdateDelay),
	fAutoUpdate(NULL),
	fDelayUpdateAfterReconnection(NULL),
	fConnected(false)
{
	_ApplyState(settings);
	_Init();
}


ForecastView::~ForecastView()
{
	StopReload();
	_DeleteBitmaps();
	delete fAutoUpdate;
}


void
ForecastView::_Init()
{
	_LoadBitmaps();
	fCondition = 34;
	// Icon for weather
	fConditionButton = new TransparentButton("condition", "",
		new BMessage(kUpdateMessage));
	fConditionButton->SetIcon(fFewClouds[LARGE_ICON]);
	fConditionButton->SetFlat(true);

	// Description (e.g. "Mostly showers", "Cloudy", "Sunny").
	BFont bold_font(be_bold_font);
	bold_font.SetSize(18);
	fConditionView = new LabelView("description",
		B_TRANSLATE("Loading" B_UTF8_ELLIPSIS));
	fConditionView->SetFont(&bold_font);

	BFont plain_font(be_plain_font);
	plain_font.SetSize(16);
	// Temperature (e.g. high 32 degrees C)
	fTemperatureView = new LabelView("temperature", "--");
	fTemperatureView->SetFont(&plain_font);

	// City
	fCityView = new LabelView("city", "--");
	fCityView->SetFont(&plain_font);
	SetCityName(fCity);

	fForecastView = new BGroupView(B_HORIZONTAL);
	BGroupLayout* forecastLayout = fForecastView->GroupLayout();
	forecastLayout->SetInsets(0, 2, 0, 0);
	forecastLayout->SetSpacing(2);

	for (int32 i = 0; i < kMaxForecastDay; i++) {
		fForecastDayView[i] = new ForecastDayView(BRect(0, 0, 62, 112));
		fForecastDayView[i]->SetIcon(fFewClouds[SMALL_ICON]);
		fForecastDayView[i]->SetDisplayUnit(fDisplayUnit);
		forecastLayout->AddView(fForecastDayView[i]);
	}

	if (!fShowForecast) {
		fForecastView->Hide();
	}

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(5, 5,
				5, 5)
		.AddGrid()
				.Add(fConditionButton, 0, 0)
				.AddGroup(B_VERTICAL,0, 1,0)
					.Add(fConditionView)
					.AddGroup(B_HORIZONTAL)
						.Add(fTemperatureView)
						.Add(fCityView)
					.End()
				.End()
				.Add(fForecastView, 0, 1, 2)
		.End()
		.AddGroup(B_HORIZONTAL, 0)
				.AddGlue()
				.Add(fDragger = new BDragger(this))
		.End()
	.End();

	SetViewColor(fBackgroundColor);
	fDragger->SetExplicitMinSize(BSize(kDraggerSize, kDraggerSize));
	fDragger->SetExplicitMaxSize(BSize(kDraggerSize, kDraggerSize));
}


BArchivable*
ForecastView::Instantiate(BMessage* archive)
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
	fUpdateDelay(kMaxUpdateDelay),
	fAutoUpdate(NULL),
	fDelayUpdateAfterReconnection(NULL),
	fConnected(false)
{
	_ApplyState(archive);
	// Use _Init to rebuild the View with deep = false in Archive
	_Init();
}


status_t
ForecastView::_ApplyState(BMessage* archive)
{
	if (archive->FindString("city", &fCity)!= B_OK)
		fCity = kDefaultCityName;

	if (archive->FindString("cityId", &fCityId)!= B_OK)
		fCityId = kDefaultCityId;

	int32 unit;
	if (archive->FindInt32("displayUnit", &unit) != B_OK) {
		bool fahrenheit;
		if (archive->FindBool("fahrenheit", &fahrenheit) == B_OK) {
			if (fahrenheit) {
				fDisplayUnit = FAHRENHEIT;
			} else {
				fDisplayUnit = CELSIUS;
			}
		} else {
			if (IsFahrenheitDefault()) {
				fDisplayUnit = FAHRENHEIT;
			} else {
				fDisplayUnit = CELSIUS;
			}
		}
	} else {
		fDisplayUnit = (DisplayUnit) unit;
	}

	if (archive->FindBool("showForecast", &fShowForecast) != B_OK)
		fShowForecast = kDefaultShowForecast;

	rgb_color *color;
	ssize_t colorsize;
	status_t status;

	status = archive->FindData("backgroundColor", B_RGB_COLOR_TYPE, (const void **)&color, &colorsize);
	fBackgroundColor = (status == B_NO_ERROR) ? *color : (fReplicated ? B_TRANSPARENT_COLOR : ui_color(B_PANEL_BACKGROUND_COLOR));

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
	status = into->AddInt32("displayUnit", (int32)fDisplayUnit);
	if (status != B_OK)
		return status;
	status = into->AddBool("showForecast", fShowForecast);
	if (status != B_OK)
		return status;

	if (!IsDefaultColor() || fReplicated) {
		status = into->AddData("textColor", B_RGB_COLOR_TYPE, &fTextColor, sizeof(rgb_color));
		if (status != B_OK)
			return status;
		status = into->AddData("backgroundColor", B_RGB_COLOR_TYPE, &fBackgroundColor, sizeof(rgb_color));
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


void
ForecastView::AttachedToWindow()
{
	if (fReplicated)
		fConditionButton->SetTarget(BMessenger(this));

	BMessenger view(this);
	BMessage autoUpdateMessage(kAutoUpdateMessage);
	fAutoUpdate = new BMessageRunner(view,  &autoUpdateMessage, (bigtime_t)fUpdateDelay * 60 * 1000 * 1000);
	fConnected = _NetworkConnected();
	if (!fConnected) {
		SetCondition(B_TRANSLATE("No network"));
		start_watching_network(
		B_WATCH_NETWORK_INTERFACE_CHANGES | B_WATCH_NETWORK_LINK_CHANGES, this);
	}
	else
		view.SendMessage(new BMessage(kUpdateMessage));
	BView::AttachedToWindow();
}


void
ForecastView::AllAttached()
{
	BView::AllAttached();
	SetTextColor(fTextColor);
	if (!_SupportTransparent() && fBackgroundColor == B_TRANSPARENT_COLOR)
			fBackgroundColor = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetBackgroundColor(fBackgroundColor);
}


void
ForecastView::Draw(BRect updateRect)
{
	if (fBackgroundColor != B_TRANSPARENT_COLOR) {
		SetHighColor(fBackgroundColor);
		FillRect(updateRect);
	}
}


bool
ForecastView::_SupportTransparent() {
	return 	fReplicated && Parent() && (Parent()->Flags() & B_DRAW_ON_CHILDREN) != 0;
}

void
ForecastView::MessageReceived(BMessage *msg)
{
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
			popup->AddItem(item = new BMenuItem(B_TRANSLATE("Background"), new BMessage('BACC')));
			popup->AddItem(item = new BMenuItem(B_TRANSLATE("Text"), new BMessage('TEXC')));
			popup->AddSeparatorItem();
			popup->AddItem(item = new BMenuItem(B_TRANSLATE("Default"), new BMessage('DEFT')));
			item->SetEnabled(!IsDefaultColor());
			if (_SupportTransparent()) {
				popup->AddItem(item = new BMenuItem(B_TRANSLATE("Transparent"), new BMessage('TRAS')));
				item->SetEnabled(ViewColor() != B_TRANSPARENT_COLOR);
			}
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
			if (item && item->Message()->what == 'TRAS') {
				SetBackgroundColor(B_TRANSPARENT_COLOR);
			}
			return;
		}
	}

	uint32 what = msg->what;
	switch (msg->what) {
	case kDataMessage: {
		BString text("");

		msg->FindInt32("temp", &fTemperature);
		msg->FindInt32("condition", &fCondition);
		msg->FindString("text", &text);

		BString tempText = FormatString(fDisplayUnit,fTemperature);
		fTemperatureView->SetText(tempText.String());
		SetCondition(_GetWeatherMessage(fCondition));
		fConditionButton->SetIcon(GetWeatherIcon(fCondition, LARGE_ICON));
		break;
	}
	case kForecastDataMessage: {
		BString text("");
		int32 forecastNum;
		BString day;
		int32 condition;
		int32 high, low;

		msg->FindInt32("forecast", &forecastNum);
		msg->FindInt32("high", &high);
		msg->FindInt32("low", &low);
		msg->FindInt32("condition", &condition);
		msg->FindString("text", &text);
		msg->FindString("day", &day);

		text = _GetWeatherMessage(condition);
		day = _GetDayText(day);

		if (forecastNum < 0 || forecastNum >= kMaxForecastDay)
			break;

		fForecastDayView[forecastNum]->SetDayLabel(day);
		fForecastDayView[forecastNum]->SetIcon(GetWeatherIcon(condition, SMALL_ICON));
		fForecastDayView[forecastNum]->SetHighTemp(high);
		fForecastDayView[forecastNum]->SetLowTemp(low);
		fForecastDayView[forecastNum]->SetToolTip(text);
		break;
	}
	case kFailureMessage: {
		fConnected = _NetworkConnected();
		if (!fConnected) {
			SetCondition(B_TRANSLATE("No network"));
			start_watching_network(
				B_WATCH_NETWORK_INTERFACE_CHANGES | B_WATCH_NETWORK_LINK_CHANGES, this);
		} else {
			SetCondition(B_TRANSLATE("Connection error"));
		}
		break;
	}
	case kUpdateMessage:
		if (fConnected) {
			SetCondition(B_TRANSLATE("Loading" B_UTF8_ELLIPSIS));
		}
	case kAutoUpdateMessage:
		Reload();
		break;
	case kShowForecastMessage:
		_ShowForecast(!fShowForecast);
		if (fShowForecast)
			Reload();
		break;
	case kUpdateCityName: {
		BString newCityName;
		if (msg->FindString("city", &newCityName) == B_OK)
			SetCityName(newCityName);
		break;
	}
	case kUpdateTTLMessage: {
		int32 ttl;
		msg->FindInt32("ttl", &ttl);
		SetUpdateDelay(ttl < kMaxUpdateDelay ? ttl : kMaxUpdateDelay);
		break;
	}
	case B_NETWORK_MONITOR:{
		fConnected = _NetworkConnected();
		if (fConnected) {
			SetCondition(B_TRANSLATE("Connecting" B_UTF8_ELLIPSIS));
			BMessenger view(this);
			BMessage updateMessage(kUpdateMessage);
			delete fDelayUpdateAfterReconnection;
			fDelayUpdateAfterReconnection = new BMessageRunner(view, &updateMessage, (bigtime_t) kReconnectionDelay  *1000 * 1000, 1);
			stop_watching_network(this);
		}
		break;
	}
	case B_ABOUT_REQUESTED: {
		BAlert *alert = new BAlert(B_TRANSLATE("About Weather"),
			B_TRANSLATE("Weather (The Replicant version)"),
			B_TRANSLATE("OK"));
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		break;
	}
	default:
		BView::MessageReceived(msg);
	}
}


void
ForecastView::_LoadBitmaps()
{
	_LoadIcons(fAlert, 'rGFX', "Artwork/weather_alert.hvif");
	_LoadIcons(fClearNight, 'rGFX',	"Artwork/weather_clear_night.hvif");
	_LoadIcons(fClear, 'rGFX', "Artwork/weather_clear.hvif");
	_LoadIcons(fClouds, 'rGFX',	"Artwork/weather_clouds.hvif");
	_LoadIcons(fCold, 'rGFX',	"Artwork/weather_cold.hvif");
	_LoadIcons(fLightDrizzle, 'rGFX',	"Artwork/weather_drizzle.hvif");
	_LoadIcons(fModerateDenseDrizzle, 'rGFX',	"Artwork/weather_icon.hvif");
	_LoadIcons(fFewClouds, 'rGFX', "Artwork/weather_few_clouds.hvif");
	_LoadIcons(fFog, 'rGFX', "Artwork/weather_fog.hvif");
	_LoadIcons(fFreezingDrizzle, 'rGFX', "Artwork/weather_freezing_drizzle.hvif");
	_LoadIcons(fIsolatedThunderstorm, 'rGFX', "Artwork/weather_isolated_thunderstorm.hvif");
	_LoadIcons(fLightSnow, 'rGFX', "Artwork/weather_light_snow.hvif");
	_LoadIcons(fMixedSnowRain, 'rGFX', "Artwork/weather_mixed_snow_rain.hvif");
	_LoadIcons(fMostlyCloudyNight, 'rGFX', "Artwork/weather_mostly_cloudy_night.hvif");
	_LoadIcons(fNightFewClouds, 'rGFX', "Artwork/weather_night_few_clouds.hvif");
	_LoadIcons(fRainingScattered, 'rGFX', "Artwork/weather_raining_scattered.hvif");
	_LoadIcons(fRaining, 'rGFX', "Artwork/weather_raining.hvif");
	_LoadIcons(fSevereThunderstorm, 'rGFX', "Artwork/weather_severe_thunderstorm.hvif");
	_LoadIcons(fIsolatedThundershowers, 'rGFX', "Artwork/weather_isolated_thundershowers.hvif");
	_LoadIcons(fShining, 'rGFX', "Artwork/weather_shining.hvif");
	_LoadIcons(fShiny, 'rGFX', "Artwork/weather_shiny.hvif");
	_LoadIcons(fSnow, 'rGFX', "Artwork/weather_snow.hvif");
	_LoadIcons(fStorm, 'rGFX', "Artwork/weather_storm.hvif");
	_LoadIcons(fThunder, 'rGFX', "Artwork/weather_thunder.hvif");
	_LoadIcons(fTornado, 'rGFX', "Artwork/weather_tornado.hvif");
	_LoadIcons(fTropicalStorm, 'rGFX', "Artwork/weather_tropical_storm.hvif");
	_LoadIcons(fCloud, 'rGFX', "Artwork/weather_cloud.hvif");
	_LoadIcons(fPartlyCloudy, 'rGFX', "Artwork/weather_partly_cloudy.hvif");
	_LoadIcons(fHurricane, 'rGFX', "Artwork/weather_hurricane.hvif");
	_LoadIcons(fSmoky, 'rGFX', "Artwork/weather_smoky.hvif");
	_LoadIcons(fScatteredSnowShowers, 'rGFX', "Artwork/weather_scattered_snow_showers.hvif");
	_LoadIcons(fSnowShowers, 'rGFX', "Artwork/weather_snow_showers.hvif");
}



void
ForecastView::_DeleteBitmaps()
{
	_DeleteIcons(fAlert);
	_DeleteIcons(fClearNight);
	_DeleteIcons(fClear);
	_DeleteIcons(fClouds);
	_DeleteIcons(fCold);
	_DeleteIcons(fLightDrizzle);
	_DeleteIcons(fModerateDenseDrizzle);
	_DeleteIcons(fFewClouds);
	_DeleteIcons(fFog);
	_DeleteIcons(fFreezingDrizzle);
	_DeleteIcons(fIsolatedThunderstorm);
	_DeleteIcons(fLightSnow);
	_DeleteIcons(fMixedSnowRain);
	_DeleteIcons(fMostlyCloudyNight);
	_DeleteIcons(fNightFewClouds);
	_DeleteIcons(fRainingScattered);
	_DeleteIcons(fRaining);
	_DeleteIcons(fIsolatedThundershowers);
	_DeleteIcons(fSevereThunderstorm);
	_DeleteIcons(fShining);
	_DeleteIcons(fShiny);
	_DeleteIcons(fSnow);
	_DeleteIcons(fStorm);
	_DeleteIcons(fThunder);
	_DeleteIcons(fTornado);
	_DeleteIcons(fTropicalStorm);
	_DeleteIcons(fCloud);
	_DeleteIcons(fHurricane);
	_DeleteIcons(fScatteredSnowShowers);
	_DeleteIcons(fSmoky);
	_DeleteIcons(fSnowShowers);
}


void
ForecastView::_DeleteIcons(BBitmap* bitmap[3])
{
	delete bitmap[0];
	delete bitmap[1];
	delete bitmap[2];
	bitmap[0] = NULL;
	bitmap[1] = NULL;
	bitmap[2] = NULL;
}


void
ForecastView::_LoadIcons(BBitmap* bitmap[3], uint32 type, const char* name)
{
	size_t dataSize;

	bitmap[0] = NULL;
	bitmap[1] = NULL;
	bitmap[2] = NULL;

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
		BBitmap* deskbarBitmap = new BBitmap(BRect(0, 0, kSizeDeskBarIcon - 1, kSizeDeskBarIcon - 1), 0,
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
		status = deskbarBitmap->InitCheck();
		if (status == B_OK) {
			status = BIconUtils::GetVectorIcon(
				reinterpret_cast<const uint8*>(data), dataSize, deskbarBitmap);
		};
		bitmap[0] = smallBitmap;
		bitmap[1] = largeBitmap;
		bitmap[2] = deskbarBitmap;
	};
}

const char *
ForecastView::_GetWeatherMessage(int32 condition)
{
//	switch (condition) {
//		case WC_TORNADO:				return B_TRANSLATE("Tornado");
//		case WC_TROPICAL_STORM:			return B_TRANSLATE("Tropical storm");
//		case WC_HURRICANE:				return B_TRANSLATE("Hurricane");
//		case WC_SEVERE_THUNDERSTORM:	return B_TRANSLATE("Severe thunderstorms");
//		case WC_STORM:					return B_TRANSLATE("Thunderstorms");
//		case WC_MIXED_SNOW_RAIN:		return B_TRANSLATE("Mixed rain and snow");
//		case WC_FREEZING_DRIZZLE:		return B_TRANSLATE("Freezing drizzle");
//		case WC_DRIZZE:					return B_TRANSLATE("Drizzle");
//		case WC_RAINING: 				return B_TRANSLATE("Raining");
//		case WC_RAINING_SCATTERED: 		return B_TRANSLATE("Showers");
//		case WC_LIGHT_SNOW: 			return B_TRANSLATE("Light snow showers");
//		case WC_SNOW: 					return B_TRANSLATE("Snow");
//		case WC_FOG: 					return B_TRANSLATE("Fog");
//		case WC_SMOKY: 					return B_TRANSLATE("Smoky");
//		case WC_WINDY: 					return B_TRANSLATE("Windy");
//		case WC_COLD: 					return B_TRANSLATE("Cold");
//		case WC_CLOUD: 					return B_TRANSLATE("Cloudy");
//		case WC_MOSTLY_CLOUDY_NIGHT: 	return B_TRANSLATE("Mostly cloudy");
//		case WC_MOSTLY_CLOUDY_DAY: 		return B_TRANSLATE("Mostly cloudy");
//		case WC_FEW_CLOUDS: 			return B_TRANSLATE("Partly cloudy");
//		case WC_CLEAR_NIGHT: 			return B_TRANSLATE("Clear");
//		case WC_ISOLATED_THUNDERSTORM: 	return B_TRANSLATE("Isolated thunderstorms");
//		case WC_SCATTERED_SNOW_SHOWERS: return B_TRANSLATE("Scattered showers");
//		case WC_SNOW_SHOWERS: 			return B_TRANSLATE("Snow showers");
//		case WC_ISOLATED_THUNDERSHOWERS:return B_TRANSLATE("Isolated thundershowers");
//		case WC_NOT_AVALIABLE: 			return B_TRANSLATE("Not available");
//	}
	
	switch (condition) {
		case WC_CLEAR_SKY:					return B_TRANSLATE("Clear sky");
		case WC_MAINLY_CLEAR:				return B_TRANSLATE("Mainly clear");
		case WC_PARTLY_CLOUDY:				return B_TRANSLATE("Partly cloudy");
		case WC_OVERCAST:					return B_TRANSLATE("Overcast");
		case WC_FOG:						return B_TRANSLATE("Fog");
		case WC_DEPOSITING_RIME_FOG:		return B_TRANSLATE("Depositing rime fog");
		case WC_LIGHT_DRIZZLE:				return B_TRANSLATE("Light drizzle");
		case WC_MODERATE_DRIZZLE:			return B_TRANSLATE("Moderate drizzle");
		case WC_DENSE_DRIZZLE:				return B_TRANSLATE("Dense drizzle");
		case WC_FREEZING_LIGHT_DRIZZLE:		return B_TRANSLATE("Freezing light drizzle");
		case WC_FREEZING_DENSE_DRIZZLE:		return B_TRANSLATE("Freezing dense drizze");
		case WC_SLIGHT_RAIN:				return B_TRANSLATE("Slight rain");
		case WC_MODERATE_RAIN:				return B_TRANSLATE("Moderate rain");
		case WC_HEAVY_RAIN:					return B_TRANSLATE("Heavy rain");
		case WC_LIGHT_FREEZING_RAIN:		return B_TRANSLATE("Light freezing rain");
		case WC_HEAVY_FREEZING_RAIN:		return B_TRANSLATE("Heavy freezing rain");
		case WC_SLIGHT_SNOW_FALL:			return B_TRANSLATE("Slight snow fall");
		case WC_MODERATE_SNOW_FALL:			return B_TRANSLATE("Moderate snow fall");
		case WC_HEAVY_SNOW_FALL:			return B_TRANSLATE("Heavy snow fall");
		case WC_SNOW_GRAINS:				return B_TRANSLATE("Snow grains");
		case WC_SLIGHT_RAIN_SHOWERS:		return B_TRANSLATE("Slight rain showers");
		case WC_MODERATE_RAIN_SHOWERS:		return B_TRANSLATE("Moderate rain showers");
		case WC_HEAVY_RAIN_SHOWERS:			return B_TRANSLATE("heavy rain showers");
		case WC_SLIGHT_SNOW_SHOWERS:		return B_TRANSLATE("Slight snow showers");
		case WC_HEAVY_SNOW_SHOWERS:			return B_TRANSLATE("Heavy snow showers");
		case WC_THUNDERSTORM:				return B_TRANSLATE("Thunderstorm");
		case WC_THUNDERSTORM_SLIGHT_HAIL:	return B_TRANSLATE("Thunderstorm with slight hail");
		case WC_THUNDERSTORM_HEAVY_HAIL:	return B_TRANSLATE("Thunderstorm with heavey hail");
	}
	return B_TRANSLATE("Not available");
}

BBitmap*
ForecastView::GetWeatherIcon(weatherIconSize iconSize)
{
	return GetWeatherIcon(fCondition, iconSize);
}

int32 ForecastView::Temperature()
{
	return fTemperature;
}

BBitmap*
ForecastView::GetWeatherIcon(int32 condition, weatherIconSize iconSize)
{
//	switch (condition) {
//		case WC_TORNADO:				return fTornado[iconSize];
//		case WC_TROPICAL_STORM:			return fTropicalStorm[iconSize];
//		case WC_HURRICANE:				return fHurricane[iconSize];
//		case WC_SEVERE_THUNDERSTORM:	return fSevereThunderstorm[iconSize];
//		case WC_STORM:					return fStorm[iconSize];
//		case WC_MIXED_SNOW_RAIN:		return fMixedSnowRain[iconSize];
//		case WC_SNOW:					return fSnow[iconSize];
//		case WC_FREEZING_DRIZZLE:		return fFreezingDrizzle[iconSize];
//		case WC_DRIZZE:					return fDrizzle[iconSize];
//		case WC_RAINING: 				return fRaining[iconSize];
//		case WC_LIGHT_SNOW: 			return fLightSnow[iconSize];
//		case WC_FOG: 					return fFog[iconSize];
//		case WC_SMOKY: 					return fSmoky[iconSize];
//		case WC_WINDY: 					return fWindy[iconSize];
//		case WC_CLOUD: 					return	fCloud[iconSize];
//		case WC_MOSTLY_CLOUDY_NIGHT: 	return fMostlyCloudyNight[iconSize];
//		case WC_MOSTLY_CLOUDY_DAY: 		return fClouds[iconSize];
//		case WC_NIGHT_FEW_CLOUDS: 		return fNightFewClouds[iconSize];
//		case WC_CLEAR_NIGHT:			return fClearNight[iconSize];
//		case WC_FEW_CLOUDS: 			return fFewClouds[iconSize];
//		case WC_RAINING_SCATTERED: 		return fRainingScattered[iconSize];
//		case WC_SHINING: 				return fShining[iconSize];
//		case WC_SCATTERED_SNOW_SHOWERS: return fScatteredSnowShowers[iconSize];
//		case WC_SNOW_SHOWERS: 			return fSnowShowers[iconSize];
//		case WC_ISOLATED_THUNDERSHOWERS:return fIsolatedThundershowers[iconSize];
//		case WC_NOT_AVALIABLE: break;
//	}
	
	switch (condition) {
		
		case WC_CLEAR_SKY: 					return fClear[iconSize];
		case WC_MAINLY_CLEAR: 				return fFewClouds[iconSize];
		case WC_PARTLY_CLOUDY: 				return fPartlyCloudy[iconSize];
		case WC_OVERCAST: 					return fClouds[iconSize];
		
		case WC_FOG: 						return fFog[iconSize];
		case WC_DEPOSITING_RIME_FOG: 		return fSmoky[iconSize];
		
		case WC_LIGHT_DRIZZLE:				return fLightDrizzle[iconSize];
		case WC_MODERATE_DRIZZLE:			return fModerateDenseDrizzle[iconSize];
		case WC_DENSE_DRIZZLE:				return fModerateDenseDrizzle[iconSize];
		case WC_FREEZING_LIGHT_DRIZZLE:		return fFreezingDrizzle[iconSize];
		case WC_FREEZING_DENSE_DRIZZLE:		return fFreezingDrizzle[iconSize];
		
		case WC_SLIGHT_RAIN: 				return fRainingScattered[iconSize];
		case WC_MODERATE_RAIN:				return fRaining[iconSize];		
		case WC_HEAVY_RAIN:					return fIsolatedThundershowers[iconSize];
		
		case WC_SLIGHT_RAIN_SHOWERS: 		return fRainingScattered[iconSize];	
		case WC_MODERATE_RAIN_SHOWERS:		return fIsolatedThundershowers[iconSize];
		case WC_HEAVY_RAIN_SHOWERS:			return fIsolatedThundershowers[iconSize];
		
		case WC_LIGHT_FREEZING_RAIN:		return fMixedSnowRain[iconSize];
		case WC_HEAVY_FREEZING_RAIN:		return fSnow[iconSize];
		
		case WC_SLIGHT_SNOW_FALL:			return fSnowShowers[iconSize];
		case WC_MODERATE_SNOW_FALL:			return fScatteredSnowShowers[iconSize];
		case WC_HEAVY_SNOW_FALL:			return fSnow[iconSize];
		
		case WC_SNOW_GRAINS:				return fMixedSnowRain[iconSize];
		
		case WC_SLIGHT_SNOW_SHOWERS:		return fScatteredSnowShowers[iconSize];
		case WC_HEAVY_SNOW_SHOWERS:			return fSnowShowers[iconSize];
		
		case WC_THUNDERSTORM:				return fIsolatedThunderstorm[iconSize];
		case WC_THUNDERSTORM_SLIGHT_HAIL:	return fAlert[iconSize];
		case WC_THUNDERSTORM_HEAVY_HAIL:	return fSevereThunderstorm[iconSize];
		
	}
	return NULL; // Change to N/A
}

BString
ForecastView::_GetDayText(const BString& dayName) const
{
	BWeekday day;
	if (strcmp(dayName.String(), "Mon") == 0) day = B_WEEKDAY_MONDAY;
	if (strcmp(dayName.String(), "Tue") == 0) day = B_WEEKDAY_TUESDAY;
	if (strcmp(dayName.String(), "Wed") == 0) day = B_WEEKDAY_WEDNESDAY;
	if (strcmp(dayName.String(), "Thu") == 0) day = B_WEEKDAY_THURSDAY;
	if (strcmp(dayName.String(), "Fri") == 0) day = B_WEEKDAY_FRIDAY;
	if (strcmp(dayName.String(), "Sat") == 0) day = B_WEEKDAY_SATURDAY;
	if (strcmp(dayName.String(), "Sun") == 0) day = B_WEEKDAY_SUNDAY;
	BString translateDayName;
	status_t result = fDateFormat.GetDayName(day, translateDayName, B_LONG_DATE_FORMAT);
	return result == B_OK  ? translateDayName : dayName;
}

void
ForecastView::SetCityId(BString cityId)
{
	fCityId = cityId;
}


BString
ForecastView::CityId()
{
	return fCityId;
}


void
ForecastView::SetCityName(BString city)
{
	fCity = city;
    fCityView->TruncateString(&city, B_TRUNCATE_END, 150);
    if (city != fCity)
		fCityView->SetToolTip(fCity);
    else
		fCityView->SetToolTip("");
	fCityView->SetText(city);
}


BString
ForecastView::CityName()
{
	return fCity;
}


void
ForecastView::SetUpdateDelay(int32 delay)
{
	if (fUpdateDelay != delay) {
		fUpdateDelay = delay;
		fAutoUpdate->SetInterval((bigtime_t)fUpdateDelay * 60 * 1000 * 1000);
	}
}


int32
ForecastView::UpdateDelay()
{
	return fUpdateDelay;
}


void
ForecastView::SetDisplayUnit(DisplayUnit unit)
{
	BString tempString;
	fDisplayUnit = unit;

	tempString = FormatString(fDisplayUnit,fTemperature);

	fTemperatureView->SetText(tempString);

	for (int32 i = 0; i < kMaxForecastDay; i++)
	{
		fForecastDayView[i]->SetDisplayUnit(fDisplayUnit);
	}
}


DisplayUnit
ForecastView::Unit()
{
	return fDisplayUnit;
}


// TODO replace this function when the localekit one is available
bool
ForecastView::IsFahrenheitDefault()
{
	BFormattingConventions conventions;
	if (BLocale::Default()->GetFormattingConventions(&conventions)
			== B_OK && conventions.CountryCode() != NULL) {
		if (strcmp(conventions.CountryCode(), "US") == 0)
			return true;
		if (strcmp(conventions.CountryCode(), "BS") == 0)
			return true;
		if (strcmp(conventions.CountryCode(), "BZ") == 0)
			return true;
		if (strcmp(conventions.CountryCode(), "PW") == 0)
			return true;
		if (strcmp(conventions.CountryCode(), "KY") == 0)
			return true;
		if (strcmp(conventions.CountryCode(), "GU") == 0)
			return true;
		if (strcmp(conventions.CountryCode(), "PR") == 0)
			return true;
		if (strcmp(conventions.CountryCode(), "VI") == 0)
			return true;
	}
	return false;
}

int32
ForecastView::GetCondition()
{
	return fCondition;
}

BString
ForecastView::GetStatus()
{
	return fConditionView->Text();
}

void
ForecastView::SetCondition(BString condition)
{
	BString conditionTruncated(condition);
	fConditionView->TruncateString(&conditionTruncated, B_TRUNCATE_END, 196);
    if (conditionTruncated != condition)
		fConditionView->SetToolTip(condition);
    else
		fConditionView->SetToolTip("");
	fConditionView->SetText(conditionTruncated);
}


void
ForecastView::SetShowForecast(bool showForecast)
{
	_ShowForecast(showForecast);
}


bool
ForecastView::ShowForecast()
{
	return fShowForecast;
}


void
ForecastView::Reload(bool forcedForecast)
{
	if (!fConnected)
		return;

	StopReload();

	fForcedForecast = forcedForecast;

	fDownloadThread = spawn_thread(&_DownloadDataFunc,
		"Download Data", B_NORMAL_PRIORITY, this);
	if (fDownloadThread >= 0)
		resume_thread(fDownloadThread);
}


void
ForecastView::StopReload()
{
	if (fDownloadThread < 0)
		return;
	wait_for_thread(fDownloadThread, NULL);
	fDownloadThread = -1;
}


int32
ForecastView::_DownloadDataFunc(void *cookie)
{
	ForecastView* forecastView = static_cast<ForecastView*>(cookie);
	forecastView->_DownloadData();
	return 0;
}


void
ForecastView::_DownloadData()
{

	// BString urlString("https://www.metaweather.com/api/location/");
	// urlString << fCityId << "/";
	WSOpenMeteo listener(this, WEATHER_REQUEST);
	BString urlString = listener.GetUrl("");
	
	BUrlRequest* request =
		BUrlProtocolRoster::MakeRequest(BUrl(urlString.String()),
		&listener);

	thread_id thread = request->Run();
	wait_for_thread(thread, NULL);
	delete request;
}


void
ForecastView::_ShowForecast(bool show)
{
	if (fShowForecast == show)
		return;
	fShowForecast = show;

	if (fShowForecast)
		fForecastView->Show();
	else
		fForecastView->Hide();
}


void
ForecastView::SetTextColor(rgb_color color)
{
	fTextColor = color;
	SetHighColor(color);
	fConditionButton->SetHighColor(color);
	fConditionView->SetHighColor(color);
	fTemperatureView->SetHighColor(color);
	fCityView->SetHighColor(color);
	fForecastView->SetHighColor(color);
	for (int32 i = 0; i < kMaxForecastDay; i++) {
		fForecastDayView[i]->SetTextColor(color);
	}
	fConditionButton->Invalidate();
	fConditionView->Invalidate();
	fTemperatureView->Invalidate();
	fCityView->Invalidate();
	fForecastView->Invalidate();
}


void
ForecastView::SetBackgroundColor(rgb_color color)
{
	fBackgroundColor = color;
	SetViewColor(color);
	fConditionButton->SetViewColor(color);
	fConditionView->SetViewColor(color);
	fTemperatureView->SetViewColor(color);
	fCityView->SetViewColor(color);
	fForecastView->SetViewColor(color);
	for (int32 i = 0; i < kMaxForecastDay; i++) {
		fForecastDayView[i]->SetViewColor(color);
		fForecastDayView[i]->Invalidate();
	}
	fConditionButton->Invalidate();
	fConditionView->Invalidate();
	fTemperatureView->Invalidate();
	fCityView->Invalidate();
	fForecastView->Invalidate();
	Invalidate();
}


bool
ForecastView::IsDefaultColor() const
{
	return fBackgroundColor == ui_color(B_PANEL_BACKGROUND_COLOR)
		&& fTextColor == ui_color(B_PANEL_TEXT_COLOR);
}


bool
ForecastView::IsConnected() const
{
	return fConnected;
}


BString
FormatString(DisplayUnit unit, int32 temp)
{
	BString tempString="";
	switch (unit) {
		case CELSIUS : {
			tempString << static_cast<int>(floor((temp - 32) * 5/9)) << "°C";
			break;
		}
		case FAHRENHEIT : {
			tempString << temp << "°F";
			break;
		}
		case KELVIN : {
			tempString << static_cast<int>(floor((temp + 459.67) * 5/9)) << "K";
			break;
		}
		case RANKINE : {
			tempString << static_cast<int>(floor(temp + 459.67)) << "°R";
			break;
		}
		case DELISLE : {
			tempString << static_cast<int>(floor((212 - temp) * 5/6)) << "°D";
			break;
		}
		default: {
			tempString << static_cast<int>(floor((temp - 32) * 5/9)) << "°C";
			break;
		}
	}
	return tempString;
}


bool
ForecastView::_NetworkConnected()
{
	BNetworkRoster& roster = BNetworkRoster::Default();
	BNetworkInterface interface;
	uint32 cookie = 0;
	while (roster.GetNextInterface(&cookie, interface) == B_OK) {
		uint32 flags = interface.Flags();
		if ((flags & IFF_LOOPBACK) == 0) {
			if ((flags & (IFF_UP | IFF_LINK)) == (IFF_UP | IFF_LINK)) {
				return true;
			}
		}
	}
	return false;
}
