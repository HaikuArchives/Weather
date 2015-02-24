/*
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Button.h>
#include <GroupLayout.h>
#include <GroupView.h>
#include <StringView.h>
#include <Window.h>

#include "MainWindow.h"
#include "PreferencesWindow.h"


PreferencesWindow::PreferencesWindow(BRect frame, MainWindow* parent,
	int32 updateDelay, bool fahrenheit)
	:
	BWindow(frame, "Preferences", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS
		| B_CLOSE_ON_ESCAPE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	fParent = parent;
	fUpdateDelay = updateDelay;
	fFahrenheit = fahrenheit;

	BGroupLayout *root = new BGroupLayout(B_VERTICAL);
	this->SetLayout(root);
	
	BGroupView *view = new BGroupView(B_VERTICAL);
	BGroupLayout *layout = view->GroupLayout();
	layout->SetInsets(16);
	this->AddChild(view);

	fFahrenheitBox = new BCheckBox("fahrenheit", "Use Fahrenheit degrees", NULL);
	fFahrenheitBox->SetValue(fFahrenheit);
	
	layout->AddView(fFahrenheitBox);
	layout->AddView(new BButton("save", "Save", new BMessage(kSavePrefMessage)));
}


void
PreferencesWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case kSavePrefMessage:
		_UpdatePreferences();
		QuitRequested();
		Quit();
		break;
	default:
		BWindow::MessageReceived(msg);
	}
}


void
PreferencesWindow::_UpdatePreferences()
{
	BMessenger messenger(fParent);
	BMessage* message = new BMessage(kUpdatePrefMessage);
	
	message->AddBool("fahrenheit", fFahrenheitBox->Value());
	
	messenger.SendMessage(message);
}


bool
PreferencesWindow::QuitRequested() {
	BMessenger messenger(fParent);
	BMessage* message = new BMessage(kClosePrefWindowMessage);
	messenger.SendMessage(message);
	return true;
}
