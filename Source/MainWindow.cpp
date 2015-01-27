/*
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * Copyright 2014 George White
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <AppKit.h>
#include <Bitmap.h>
#include <FindDirectory.h>
#include <Font.h>
#include <GroupLayout.h>

#include <IconUtils.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <TranslationUtils.h>
#include <Url.h>
#include <UrlProtocolRoster.h>
#include <UrlRequest.h>

#include "MainWindow.h"
#include "NetListener.h"
#include "PreferencesWindow.h"
#include "SelectionWindow.h"

#include <stdio.h>

//#include <Dragger.h>

const char* kSettingsFileName = "HaikuWeather settings";

const char* kDefaultCityName = "Menlo Park, CA";
const char* kDefaultCityId = "2449435";
const int32	kDefaultUpdateDelay = 30;
const bool	kDefaultFahrenheit = false;
const bool	kDefaultShowForecast = false;
const BRect kDefaultMainWindowRect = BRect(150,150,0,0);

BMenuBar* MainWindow::PrepareMenuBar(void) {
	BMenuBar *menubar = new BMenuBar("menu");
	BMenu *menu = new BMenu("Edit");
	menu->AddItem(new BMenuItem("Change location" B_UTF8_ELLIPSIS,
		new BMessage(kCitySelectionMessage),'L'));
	menu->AddItem(new BMenuItem("Preferences" B_UTF8_ELLIPSIS,
		new BMessage(kOpenPreferencesMessage), ',' ));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
	menubar->AddItem(menu);
	
	menu = new BMenu("View");
	menu->AddItem(fShowForecastMenuItem = new BMenuItem("Show Forecast", new BMessage(kShowForecastMessage)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Refresh", new BMessage(kUpdateMessage), 'R'));
	menubar->AddItem(menu);

	return menubar;
}


MainWindow::MainWindow()
	:
	BWindow(BRect(150, 150, 0, 0), "HaikuWeather",  B_TITLED_WINDOW, B_NOT_RESIZABLE |
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE
		| B_AUTO_UPDATE_SIZE_LIMITS),
		fSelectionWindow(NULL)
{
	BGroupLayout* root = new BGroupLayout(B_VERTICAL);
	root->SetSpacing(0);
	this->SetLayout(root);
	fView = new BGridView(3, 3);
	fLayout = fView->GridLayout();
	fLayout->SetInsets(5);
	BMenuBar *menubar = PrepareMenuBar();
	this->AddChild(menubar);
	this->AddChild(fView);

	fResources = be_app->AppResources();
	_LoadBitmaps();

	fCity = kDefaultCityName;
	fCityId = kDefaultCityId;
	fUpdateDelay = kDefaultUpdateDelay;
	fFahrenheit = kDefaultFahrenheit;
	fShowForecast = kDefaultShowForecast;
	fMainWindowRect = kDefaultMainWindowRect;

	_LoadSettings();

	MoveTo(fMainWindowRect.LeftTop());

	BMessenger window(this);
	BMessage autoUpdateMessage(kAutoUpdateMessage);
	fAutoUpdate = new BMessageRunner(window,  &autoUpdateMessage, fUpdateDelay * 60 * 1000 * 1000);
	window.SendMessage(new BMessage(kUpdateMessage));

	// Icon for weather
	fConditionButton = new BButton("condition", "",
		new BMessage(kUpdateMessage));
	fConditionButton->SetIcon(fFewClouds);
	fLayout->AddView(fConditionButton, (int32) 0, (int32) 0);
	
	BGroupView *infoView = new BGroupView(B_VERTICAL);
	BGroupLayout *infoLayout = infoView->GroupLayout();
	infoLayout->SetInsets(16);
	fLayout->AddView(infoView, (int32) 1, (int32) 0);
	
	// Description (e.g. "Mostly showers", "Cloudy", "Sunny").
	BFont* bold_font = new BFont(be_bold_font);
	bold_font->SetSize(24);
	fConditionView = new BStringView("description", "Loading" B_UTF8_ELLIPSIS);
	fConditionView->SetFont(bold_font);
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
	//infoLayout->AddView(forecastView);
	this->AddChild(fForecastView);

	for (int i = 0; i < 5 /*MAX_FORECAST_DAY*/; i++)
	{
		fForecastButton[i] = new BButton("condition", "",
									new BMessage(kUpdateMessage));
		fForecastButton[i]->SetIcon(fFewClouds);
		forecastLayout->AddView(fForecastButton[i]);
	}

	if (!fShowForecast) {
		fForecastView->Hide();
	}
	fShowForecastMenuItem->SetMarked(fShowForecast);
}

void MainWindow::MessageReceived(BMessage *msg) {
	BString tempString("");
	BString text("");
	int32 tempDelay;
	bool tempFahrenheit;

	//msg->PrintToStream();

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
		fConditionButton->SetIcon(_GetWeatherIcon(fCondition));
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

		if (forecastNum < 0 || forecastNum > 5 /*MAX_DAY_FORECAST*/)
			break;

		BString highString = "";
		
		if (fFahrenheit)
			highString << high << "°F";
		else
			highString << static_cast<int>(floor(CEL(high))) << "°C";

		BString lowString = "";
		if (fFahrenheit)
			lowString << low << "°F";
		else
			lowString << static_cast<int>(floor(CEL(low))) << "°C";

		fForecastButton[forecastNum]->SetLabel(day);
		BString toolTip = text << "\n" << lowString << "/" << highString; // << " (" << condition << ")";
		fForecastButton[forecastNum]->SetToolTip(toolTip);
		fForecastButton[forecastNum]->SetIcon(_GetWeatherIcon(condition));
		break;
	}
	case kUpdateCityMessage:
		tempString = fCityId;
		msg->FindString("city", &fCity);
		msg->FindString("id", &fCityId);

		if (fCityId != tempString) {
			fCityView->SetText(fCity);
			fConditionView->SetText("Loading" B_UTF8_ELLIPSIS);
			// forcedForecast retrieve full city name
			_DownloadData(true);
		}

		_SaveSettings();
		break;
	case kUpdateCityName:{
		BString newCityName;
		if (msg->FindString("city", &newCityName) == B_OK && newCityName != fCity) {
			fCity = newCityName;
			fCityView->SetText(fCity);
			_SaveSettings();
		}}
		break;

	case kFailureMessage:
		fConditionView->SetText("Connection error");
		break;

	case kUpdatePrefMessage:
		tempDelay = fUpdateDelay;
		tempFahrenheit = fFahrenheit;

		msg->FindInt32("delay", &fUpdateDelay);
		msg->FindBool("fahrenheit", &fFahrenheit);

		if (fFahrenheit != tempFahrenheit) {
			fConditionView->SetText("Loading" B_UTF8_ELLIPSIS);
			_DownloadData();
		}

		if (fUpdateDelay != tempDelay) {
			fAutoUpdate->SetInterval(fUpdateDelay * 60 * 1000 * 1000);
		}

		_SaveSettings();
		break;
	case kUpdateMessage:
		fConditionView->SetText("Loading" B_UTF8_ELLIPSIS);
	case kAutoUpdateMessage:
		_DownloadData();
		break;
	case kShowForecastMessage:
		_ShowForecast(!fShowForecast);
		if (fShowForecast)
			_DownloadData();
		_SaveSettings();
		break;
	case kCitySelectionMessage:
		if (fSelectionWindow == NULL){
			BRect frame(Frame().LeftTop(),BSize(400,200));
			frame.OffsetBy(30,30);
			fSelectionWindow = new SelectionWindow(frame, this, fCity, fCityId);
			fSelectionWindow->Show();
		}
		else {
			BRect frame(Frame().LeftTop(),BSize(400,200));
			frame.OffsetBy(30,30);
			fSelectionWindow->MoveTo(frame.LeftTop());
			if (fSelectionWindow->IsHidden())
				fSelectionWindow->Show();
			else
				fSelectionWindow->Activate();
		}
		break;
	case kOpenPreferencesMessage:
		(new PreferencesWindow(this, fUpdateDelay, fFahrenheit))->Show();
		break;
	case kCloseCitySelectionWindowMessage:
		fSelectionWindow = NULL;
		break;
	}

}


void MainWindow::_LoadBitmaps() {
	fAlert = BTranslationUtils::GetBitmap('rGFX', "Artwork/weather_alert.hvif");
	fClearNight = BTranslationUtils::GetBitmap('rGFX',
		"Artwork/weather_clear_night.hvif");
	fClear = BTranslationUtils::GetBitmap('rGFX', "Artwork/weather_clear.hvif");
	fClouds = BTranslationUtils::GetBitmap('rGFX',
		"Artwork/weather_clouds.hvif");
	fFewClouds = BTranslationUtils::GetBitmap('rGFX',
		"Artwork/weather_few_clouds.hvif");
	fFog = BTranslationUtils::GetBitmap('rGFX', "Artwork/weather_fog.hvif");
	fNightFewClouds = BTranslationUtils::GetBitmap('rGFX',
		"Artwork/weather_night_few_clouds.hvif");
	fRainingScattered = BTranslationUtils::GetBitmap('rGFX',
		"Artwork/weather_raining_scattered.hvif");
	fRaining = BTranslationUtils::GetBitmap('rGFX',
		"Artwork/weather_raining.hvif");
	fShining = BTranslationUtils::GetBitmap('rGFX',
		"Artwork/weather_shining.hvif");
	fShiny = BTranslationUtils::GetBitmap('rGFX', "Artwork/weather_shiny.hvif");
	fSnow = BTranslationUtils::GetBitmap('rGFX', "Artwork/weather_snow.hvif");
	fStorm = BTranslationUtils::GetBitmap('rGFX', "Artwork/weather_storm.hvif");
	fThunder = BTranslationUtils::GetBitmap('rGFX',
		"Artwork/weather_thunder.hvif");
}

BBitmap* MainWindow::_GetWeatherIcon(int32 condition) {

	switch (condition) {
		case 0:
		case 1:
		case 2:	return fAlert;
		case 3:
		case 4:	return fStorm;
		case 5:
		case 6:
		case 7: return fSnow;
		case 8:
		case 9:
		case 11:
		case 12: return fRainingScattered;
		case 10: return fRaining;
		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
		case 18: return fSnow;
		case 20: return fFog;
		case 26:
		case 27:
		case 28: return fClouds;
		case 29: return fNightFewClouds;
		case 30: return fFewClouds;
		case 31: return fClearNight;
		case 32: return fClear;
		case 33: return fClearNight;
		case 34: return fFewClouds;
		case 35: return fRainingScattered;
		case 36: return fShining;
		case 37: return fThunder;
		case 38:
		case 39: return fStorm;
		case 40: return fRaining;
		case 41:
		case 42:
		case 43: return fSnow;
		case 45: return fStorm;
		case 46: return fSnow;
		case 47: return fThunder;
	}
	//printf("Unknow code %d\n", condition);
	return NULL; // Change to N/A

}


void MainWindow::_DownloadData(bool forcedForecast) {
	BString urlString("https://query.yahooapis.com/v1/public/yql");
	if (fShowForecast || forcedForecast)
		urlString << "?q=select+*+from+weather.forecast+";
	else
		urlString << "?q=select+item.condition+from+weather.forecast+";

	urlString << "where+woeid+=+" << fCityId << "&format=json";

	//printf("Request %s\n", urlString.String());
	BUrlRequest* request =
		BUrlProtocolRoster::MakeRequest(BUrl(urlString.String()),
		new NetListener(this, WEATHER_REQUEST));
	status_t err = request->Run();
	if (err != B_OK) ; // TODO Sent error message
}

void MainWindow::_ShowForecast(bool show) {
	if (fShowForecast == show)
		return;
	fShowForecast = show;

	if (fShowForecast)
		fForecastView->Show();
	else
		fForecastView->Hide();

	fShowForecastMenuItem->SetMarked(fShowForecast);
}


status_t MainWindow::_LoadSettings() {
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
	
	if (m.FindRect("fMainWindowRect", &fMainWindowRect) != B_OK)
		fMainWindowRect = kDefaultMainWindowRect;

	return B_OK;
}


status_t MainWindow::_SaveSettings() {
	BPath p;
	BFile f;
	BMessage m(kSettingsMessage);
	
	m.AddString("fCity", fCity);
	m.AddString("fCityId", fCityId);
	m.AddInt32("fUpdateDelay", fUpdateDelay);
	m.AddBool("fFahrenheit", fFahrenheit);
	m.AddBool("fShowForecast", fShowForecast);
	m.AddRect("fMainWindowRect", Frame());
	
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

bool MainWindow::QuitRequested()
{
	_SaveSettings();
	return true;
}
