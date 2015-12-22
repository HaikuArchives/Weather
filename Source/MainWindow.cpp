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


BMenuBar*
MainWindow::_PrepareMenuBar(void)
{
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
	// Remove Show Forecast until it works properly
//	menu->AddItem(fShowForecastMenuItem = new BMenuItem("Show Forecast", new BMessage(kShowForecastMessage)));
//	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Refresh", new BMessage(kUpdateMessage), 'R'));
	menubar->AddItem(menu);

	return menubar;
}


MainWindow::MainWindow()
	:
	BWindow(BRect(150, 150, 0, 0), "Weather",  B_TITLED_WINDOW, B_NOT_RESIZABLE
		| B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE
		| B_AUTO_UPDATE_SIZE_LIMITS),
		fSelectionWindow(NULL),
		fPreferencesWindow(NULL)
{
	BGroupLayout* root = new BGroupLayout(B_VERTICAL);
	root->SetSpacing(0);
	SetLayout(root);

	AddChild(_PrepareMenuBar());

	BMessage settings;
	_LoadSettings(settings);

	fForecastView = new ForecastView(BRect(0,0,100,100), &settings);
	AddChild(fForecastView);
	// Enable when works
//	fShowForecastMenuItem->SetMarked(fForecastView->ShowForecast());
}


void
MainWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case kUpdateCityMessage: {

		BString cityName, cityId;

		msg->FindString("city", &cityName);
		msg->FindString("id", &cityId);

		if (fForecastView->CityId() != cityId) {
			fForecastView->SetCityName(cityName);
			fForecastView->SetCityId(cityId);
			fForecastView->SetCondition("Loading" B_UTF8_ELLIPSIS);
			// forcedForecast use forecast request to retrieve full city name
			// In the condition respond the isn't the full city name
			fForecastView->Reload(true);
		}
		}
		break;
	case kUpdatePrefMessage:
		int32 unit;
		msg->FindInt32("displayUnit", &unit);
		fForecastView->SetDisplayUnit((DisplayUnit)unit);
		break;
	case kUpdateMessage:
		fForecastView->SetCondition("Loading" B_UTF8_ELLIPSIS);
		fForecastView->Reload();
		break;
	case kShowForecastMessage: {
		bool show = !fForecastView->ShowForecast();
		fForecastView->SetShowForecast(show);
		fShowForecastMenuItem->SetMarked(show);
		fForecastView->Reload();
		}
		break;
	case kCitySelectionMessage:
		if (fSelectionWindow == NULL){
			BRect frame(Frame().LeftTop(),BSize(400,200));
			frame.OffsetBy(30,30);
			fSelectionWindow = new SelectionWindow(frame, this, fForecastView->CityName(), fForecastView->CityId());
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
		if (fPreferencesWindow == NULL){
			fPreferencesWindow = new PreferencesWindow(BRect(200,200,400,200), this,
				fForecastView->UpdateDelay(), fForecastView->Unit());
			fPreferencesWindow->Show();
		}
		else {
			fPreferencesWindow->Activate();
		}
		break;
	case kCloseCitySelectionWindowMessage:
		fSelectionWindow = NULL;
		break;
	case kClosePrefWindowMessage:
		fPreferencesWindow = NULL;
		break;
	default:
		BWindow::MessageReceived(msg);
	}
}


status_t
MainWindow::_LoadSettings(BMessage& m)
{
	BPath p;
	BFile f;

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

	if (m.FindRect("fMainWindowRect", &fMainWindowRect) != B_OK)
		fMainWindowRect = kDefaultMainWindowRect;

	MoveTo(fMainWindowRect.LeftTop());

	return B_OK;
}


status_t
MainWindow::_SaveSettings()
{
	BPath p;
	BFile f;
	BMessage m(kSettingsMessage);

	fForecastView->SaveState(&m);

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
