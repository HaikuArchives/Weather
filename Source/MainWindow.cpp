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
#include <GroupView.h>
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

const char* kSettingsFileName = "HaikuWeather settings";
int fAutoUpdateDelay = 15;


BMenuBar* MainWindow::PrepareMenuBar(void) {
	BMenuBar *menubar = new BMenuBar("menu");
	BMenu *menu = new BMenu("Edit");
	
	menu->AddItem(new BMenuItem("Refresh", new BMessage(kUpdateMessage), 'r'));
	menu->AddSeparatorItem();
	
	menu->AddItem(new BMenuItem("Change location...",
		new BMessage(kCitySelectionMessage), NULL, NULL));
	menu->AddItem(new BMenuItem("Preferences...",
		new BMessage(kOpenPreferencesMessage), NULL, NULL));
	
	menubar->AddItem(menu);
	
	return menubar;
}


MainWindow::MainWindow()
	:
	BWindow(BRect(50, 50, 0, 0), "HaikuWeather", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE
		| B_AUTO_UPDATE_SIZE_LIMITS) {
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
	
	fCity = "Katowice, Poland";
	fCityId = "498842";
	fUpdateDelay = 15;
	fFahrenheit = false;
	
	_LoadSettings();
	
	fAutoUpdate = spawn_thread(autoUpdate, "autoUpdate", 10, (void*) this);
	resume_thread(fAutoUpdate);
	
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
	fConditionView = new BStringView("description", "Loading...");
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
	
}

void MainWindow::MessageReceived(BMessage *msg) {
	BString tempString("");
	BString text("");
	int32 tempDelay;
	bool tempFahrenheit;

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
		
		if (fCondition >= 0 && fCondition <= 2)
			fConditionButton->SetIcon(fAlert);
		else if (fCondition >= 3 && fCondition <= 4)
			fConditionButton->SetIcon(fStorm);
		else if (fCondition >= 5 && fCondition <= 7)
			fConditionButton->SetIcon(fSnow);
		else if (fCondition == 10)
			fConditionButton->SetIcon(fRaining);
		else if (fCondition >= 8 && fCondition <= 12)
			fConditionButton->SetIcon(fRainingScattered);
		else if (fCondition >= 13 && fCondition <= 18)
			fConditionButton->SetIcon(fSnow);
		else if (fCondition >= 26 && fCondition <= 28)
			fConditionButton->SetIcon(fClouds);
		else if (fCondition == 29)
			fConditionButton->SetIcon(fNightFewClouds);
		else if (fCondition == 31)
			fConditionButton->SetIcon(fClearNight);
		else if (fCondition == 32)
			fConditionButton->SetIcon(fClear);
		else if (fCondition == 33)
			fConditionButton->SetIcon(fClearNight);
		else if (fCondition == 34)
			fConditionButton->SetIcon(fShining);
		else if (fCondition == 35)
			fConditionButton->SetIcon(fRainingScattered);
		else if (fCondition == 36)
			fConditionButton->SetIcon(fShining);
		else if (fCondition == 37)
			fConditionButton->SetIcon(fThunder);
		else if (fCondition >= 38 && fCondition <= 39)
			fConditionButton->SetIcon(fStorm);
		else if (fCondition == 40)
			fConditionButton->SetIcon(fRaining);
		else if (fCondition >= 41 && fCondition <= 43)
			fConditionButton->SetIcon(fSnow);
		else if (fCondition == 45)
			fConditionButton->SetIcon(fStorm);
		else if (fCondition == 46)
			fConditionButton->SetIcon(fSnow);
		else if (fCondition == 47)
			fConditionButton->SetIcon(fThunder);
		break;
	case kUpdateCityMessage:
		tempString = fCityId;
		msg->FindString("city", &fCity);
		msg->FindString("id", &fCityId);
		
		if (fCityId != tempString) {
			fCityView->SetText(fCity);
			fConditionView->SetText("Loading...");
			_DownloadData();
		}
		
		_SaveSettings();
		break;
	case kUpdatePrefMessage:
		tempDelay = fUpdateDelay;
		tempFahrenheit = fFahrenheit;
		
		msg->FindInt32("delay", &fUpdateDelay);
		msg->FindBool("fahrenheit", &fFahrenheit);
		
		if (fFahrenheit != tempFahrenheit) {
			fConditionView->SetText("Loading...");
			_DownloadData();
		}
		
		if (fUpdateDelay != tempDelay) {
			kill_thread(fAutoUpdate);
			fAutoUpdate = spawn_thread(autoUpdate, "autoUpdate", 10,
				(void*) this);
			resume_thread(fAutoUpdate);
		}
		
		_SaveSettings();
		break;
	case kUpdateMessage:
		fConditionView->SetText("Loading...");
	case kAutoUpdateMessage:
		_DownloadData();
		break;
	case kCitySelectionMessage:
		(new SelectionWindow(this, fCity, fCityId))->Show();
		break;
	case kOpenPreferencesMessage:
		(new PreferencesWindow(this, fUpdateDelay, fFahrenheit))->Show();
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


void MainWindow::_DownloadData() {
	BString urlString("https://query.yahooapis.com/v1/public/yql");
	urlString << "?q=select+item.condition+from+weather.forecast+"
		<< "where+woeid+=+" << fCityId;
	
	BUrlRequest* request =
		BUrlProtocolRoster::MakeRequest(BUrl(urlString.String()),
		new NetListener(this));
	status_t err = request->Run();
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
		fCity = "Katowice, Poland";
	if (m.FindString("fCityId", &fCityId) != B_OK)
		fCityId = "498842";
	
	if (m.FindInt32("fUpdateDelay", &fUpdateDelay) != B_OK)
		fUpdateDelay = 15;
	if (m.FindBool("fFahrenheit", &fFahrenheit) != B_OK)
		fFahrenheit = false;
	
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


status_t autoUpdate(void* data) {
	while (true) {
		BMessenger* messenger = new BMessenger((MainWindow*) data);
		BMessage* message = new BMessage(kAutoUpdateMessage);
		messenger->SendMessage(message);
		
		snooze(fAutoUpdateDelay * 60 * 1000 * 1000);
	}
}
