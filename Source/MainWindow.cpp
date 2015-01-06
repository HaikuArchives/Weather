/*
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * Copyright 2014 George White
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <AppKit.h>
#include <Bitmap.h>
#include <Button.h>
#include <Font.h>
#include <GroupLayout.h>
#include <GroupView.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <IconUtils.h>
#include <StringView.h>
#include <TranslationUtils.h>

#include "MainWindow.h"
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
	view = new BGridView(3, 3);
	layout = view->GridLayout();
	layout->SetInsets(5);
	BMenuBar *menubar = PrepareMenuBar();
	this->AddChild(menubar);
	this->AddChild(view);
	
	_LoadBitmaps();
	
	// Icon for weather
	BButton *weatherButton = new BButton("");
	weatherButton->SetIcon(fCloudy);
	layout->AddView(weatherButton, (int32) 0, (int32) 0);
	
	BGroupView *infoView = new BGroupView(B_VERTICAL);
	BGroupLayout *infoLayout = infoView->GroupLayout();
	infoLayout->SetInsets(16);
	layout->AddView(infoView, (int32) 1, (int32) 0);
	
	// Description (e.g. "Mostly showers", "Cloudy", "Sunny").
	BFont *bold_font = new BFont(be_bold_font);
	bold_font->SetSize(24);
	BStringView *desc = new BStringView("description", "Cloudy.");
	desc->SetFont(bold_font);
	infoLayout->AddView(desc);
	
	// Numbers (e.g. temperature etc.)
	BGroupView *numberView = new BGroupView(B_HORIZONTAL);
	BGroupLayout *numberLayout = numberView->GroupLayout();
	infoLayout->AddView(numberView);
	
	// Temperature (e.g. high 32 degrees C)
	numberLayout->AddView(new BStringView("high temperature", "high 32°C"));
	numberLayout->AddView(new BStringView("low temperature", "low 27°C"));
	
	// Precipitation (e.g. 30%)
	numberLayout->AddView(new BStringView("precipitation", "30% precipitation"));
	
}

void MainWindow::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case 25: {
			(new SelectionWindow())->Show();
		}
		
		default: {
			// BWindow::MessageRecieved(msg);
			break;
		}
	}
}


void MainWindow::_LoadBitmaps() {
	fCloudy = BTranslationUtils::GetBitmap('rGFX', "Artwork/weather_clouds.hvif");
}

