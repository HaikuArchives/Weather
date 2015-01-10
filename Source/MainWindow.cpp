/*
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * Copyright 2014 George White
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <AppKit.h>
#include <Bitmap.h>
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

#include <stdio.h>

#include "MainWindow.h"
#include "NetListener.h"
#include "SelectionWindow.h"


BMenuBar* MainWindow::PrepareMenuBar(void) {
	BMenuBar *menubar = new BMenuBar("menu");
	BMenu *menu = new BMenu("Edit");
	
	menu->AddItem(new BMenuItem("Refresh", NULL, 'r'));
	
	BMessage *msg = new BMessage(25);
	menu->AddItem(new BMenuItem("Change location", msg, NULL, NULL));
	
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
	
	fCondition = -1;
	fTemperature = -101;
	_DownloadData(2487889);
	
	// Icon for weather
	fConditionButton = new BButton("");
	fConditionButton->SetIcon(fCloudy);
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
	fCityView = new BStringView("city", "New York");
	numberLayout->AddView(fCityView);
	
}

void MainWindow::MessageReceived(BMessage *msg) {
	BString tempString("");
	BString text("");

	switch (msg->what) {
	case kDataMessage:
		fConditionView->SetText("Test!");
		msg->FindInt32("temp", &fTemperature);
		msg->FindInt32("code", &fCondition);
		
		msg->FindString("text", &text);
		
		tempString << static_cast<int>(floor(CEL(fTemperature))) << "°C";
		fTemperatureView->SetText(tempString);
		fConditionView->SetText(text);
		
		/*switch (fCondition) {
			
		}*/
		break;
	case 25:
		(new SelectionWindow())->Show();
	// default:
		// BWindow::MessageRecieved(msg);
	}
}


void MainWindow::_LoadBitmaps() {
	fCloudy = BTranslationUtils::GetBitmap('rGFX', "Artwork/weather_clouds.hvif");
}


void MainWindow::_DownloadData(int city) {
	BString urlString("https://query.yahooapis.com/v1/public/yql");
	urlString << "?q=select+item.condition+from+weather.forecast+"
		<< "where+woeid+=+" << city;
	
	BUrlRequest* request =
		BUrlProtocolRoster::MakeRequest(BUrl(urlString.String()),
		new NetListener(this));
	status_t err = request->Run();
}
