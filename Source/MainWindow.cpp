/*
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * Copyright 2014 George White
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <AboutWindow.h>
#include <AppKit.h>
#include <Bitmap.h>
#include <Catalog.h>

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

#include <Deskbar.h>
#include <Alert.h>
#include <Roster.h>

#include "MainWindow.h"
#include "PreferencesWindow.h"
#include "SelectionWindow.h"
#include "ForecastDeskbarView.h"
#include "ForecastView.h"
#include "Util.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"

BMenuBar*
MainWindow::_PrepareMenuBar(void)
{
	BMenuBar *menubar = new BMenuBar("menu");
	BMenu *menu = new BMenu(B_TRANSLATE("Edit"));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Change location" B_UTF8_ELLIPSIS),
		new BMessage(kCitySelectionMessage),'L'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Preferences" B_UTF8_ELLIPSIS),
		new BMessage(kOpenPreferencesMessage), ',' ));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("About Weather"), 
		new BMessage(B_ABOUT_REQUESTED)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED), 'Q'));
	menubar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("View"));
	//menu->AddItem(fShowForecastMenuItem = new BMenuItem("Show forecast",
	//	new BMessage(kShowForecastMessage)));
	//menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Refresh"),
		new BMessage(kUpdateMessage), 'R'));
	menu->AddItem(fReplicantMenuItem = new BMenuItem(B_TRANSLATE("Deskbar Replicant"),
		new BMessage(kToggleDeskbarReplicantMessage), 'T'));
	menubar->AddItem(menu);

	return menubar;
}


MainWindow::MainWindow()
	:
	BWindow(BRect(150, 150, 0, 0), B_TRANSLATE_SYSTEM_NAME("Weather"),
		B_TITLED_WINDOW, B_NOT_RESIZABLE
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
	LoadSettings(settings);

	if (settings.FindRect("fMainWindowRect", &fMainWindowRect) != B_OK)
	{
		fMainWindowRect = kDefaultMainWindowRect;
	}

	MoveTo(fMainWindowRect.LeftTop());


	fForecastView = new ForecastView(BRect(0,0,100,100), &settings);
	AddChild(fForecastView);
	// Enable when works
	//fShowForecastMenuItem->SetMarked(fForecastView->ShowForecast());
}

void
MainWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case kUpdateCityMessage: {

		BString cityName;
		int32 	cityId;
		double 	latitude, longitude;

		msg->FindString("city", &cityName);
		msg->FindInt32("id", &cityId);
		msg->FindDouble("latitude", &latitude);
		msg->FindDouble("longitude", &longitude);

		if (fForecastView->CityId() != cityId) {
			fForecastView->SetCityName(cityName);
			fForecastView->SetCityId(cityId);
			fForecastView->SetLatitude(latitude);
			fForecastView->SetLongitude(longitude);
			fForecastView->SetCondition(B_TRANSLATE("Loading" B_UTF8_ELLIPSIS));
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
		if (fForecastView->IsConnected()) {
			fForecastView->SetCondition(B_TRANSLATE("Loading" B_UTF8_ELLIPSIS));
			fForecastView->Reload();
		}
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
			fSelectionWindow = new SelectionWindow(frame, this,
				fForecastView->CityName(), fForecastView->CityId());
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
			fPreferencesWindow = new PreferencesWindow(BRect(200,200,400,200),
				this, fForecastView->UpdateDelay(), fForecastView->Unit());
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
	case kToggleDeskbarReplicantMessage:
	{
		BDeskbar deskbar;

		if (!deskbar.HasItem("ForecastDeskbarView"))
		{
			app_info info;
			be_roster->GetAppInfo("application/x-vnd.przemub.Weather", &info);

			status_t result = deskbar.AddItem(&info.ref);
			if (result != B_OK)
			{
				BString errorMessage = B_TRANSLATE("Unable to create a deskbar replicant. The error is: \"");
				errorMessage << strerror(result) << "\".";

				BAlert* alert = new BAlert("Error", errorMessage.String(), "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
				alert->Go();
				delete alert;
			}
		}
		else
		{
				deskbar.RemoveItem("ForecastDeskbarView");
		}
		break;
	}
	case B_ABOUT_REQUESTED:
	{
		AboutRequested();
		break;
	}	
	default:
		BWindow::MessageReceived(msg);
	}
}

status_t
MainWindow::_SaveSettings()
{
	BPath p;
	BFile f;
	BMessage m(kSettingsMessage);

	fForecastView->SaveState(&m);

	m.AddRect("fMainWindowRect", Frame());

	app_info info;
	be_roster->GetAppInfo("application/x-vnd.przemub.Weather", &info);
	m.AddRef("appLocation", &info.ref);

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


void
MainWindow::MenusBeginning()
{
	BDeskbar deskbar;
	fReplicantMenuItem->SetMarked(deskbar.HasItem("ForecastDeskbarView"));
}


void
MainWindow::AboutRequested()
{
	BAboutWindow* about = new BAboutWindow(
		B_TRANSLATE_SYSTEM_NAME("Weather"), "application/x-vnd.przemub.Weather");

	const char* kAuthors[] = {
		"Davide Alfano (Nexus6)",
		"Adrián Arroyo Calle",
		"Akshay Agarwal",
		"Bach Nguyen",
		"Benjamin Amos",
		"George White",
		"Humdinger",
		"Janus",
		"Kevin Adams",
		"Naseef",
		"Przemyslaw Buczkowski",
		"Sergei Reznikov",
		"Stephanie Fu",
		"Venu Vardhan Reddy Tekula",
		"Waddlesplash",
		NULL
	};
		
	const char* kCopyright = "George White";
	
	about->AddDescription(B_TRANSLATE("An open source weather app showing a forecast for the next 5 days."));
	about->AddAuthors(kAuthors);
	about->AddCopyright(2014, kCopyright, NULL);
	about->Show();
}
	
bool
MainWindow::QuitRequested()
{
	_SaveSettings();
	return true;
}
