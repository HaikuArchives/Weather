/*
	Copyright (C) 2014 George White.
	Licensed under the MIT license.
	Made for Haiku.
*/

#include <MenuBar.h>
#include <Menu.h>
#include <MenuItem.h>
#include <StringView.h>
#include "MainWindow.h"

MainWindow::MainWindow(void) : BWindow(
								BRect(100, 100, 500, 400),
								"Weather",
								B_TITLED_WINDOW,
								B_ASYNCHRONOUS_CONTROLS |
								B_QUIT_ON_WINDOW_CLOSE) {
	// TODO: something on construction
	layout = new BGridLayout();
	this->SetLayout(layout);
	layout->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	layout->AddView(new BStringView(BRect(100, 10, 50, 50), "Hello", "Mostly showers."), (int32) 0, (int32) 0);
}

void MainWindow::MessageRecieved(BMessage *msg) {
	switch (msg->what) {
		default: {
			// BWindow::MessageRecieved(msg);
			break;
		}
	}
}

