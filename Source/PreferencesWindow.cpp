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


PreferencesWindow::PreferencesWindow(MainWindow* parent,
	int32 updateDelay, bool fahrenheit)
:
BWindow(BRect(50, 50, 0, 0), "Preferences",
	B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS) {
	fParent = parent;
	fUpdateDelay = updateDelay;
	fFahrenheit = fahrenheit;

	BGroupLayout *root = new BGroupLayout(B_VERTICAL);
	this->SetLayout(root);
	
	BGroupView *view = new BGroupView(B_VERTICAL);
	BGroupLayout *layout = view->GroupLayout();
	layout->SetInsets(16);
	this->AddChild(view);
	
	//fUpdateDelayControl = new BTextControl(NULL,
	//	"Auto-update delay (in minutes):", fUpdateDelay, NULL);
	fUpdateDelaySlider = new BSlider("delay", "Auto-update delay (in minutes):",
		NULL, 1, 240, B_HORIZONTAL, B_TRIANGLE_THUMB);
	fUpdateDelaySlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fUpdateDelaySlider->SetLimitLabels("1 minute", "4 hours");
	fFahrenheitBox = new BCheckBox("fahrenheit", "Use Fahrenheit degrees", NULL);
	
	fUpdateDelaySlider->SetValue(fUpdateDelay);
	fFahrenheitBox->SetValue(fFahrenheit);
	
	layout->AddView(fUpdateDelaySlider);
	layout->AddView(fFahrenheitBox);
	layout->AddView(new BButton("save", "Save", new BMessage(kSavePrefMessage)));
}


void PreferencesWindow::MessageReceived(BMessage *msg) {
	switch (msg->what) {
	case kSavePrefMessage:
		_UpdatePreferences();
		Quit();
	}
}


void PreferencesWindow::_UpdatePreferences() {
	BMessenger* messenger = new BMessenger(fParent);
	BMessage* message = new BMessage(kUpdatePrefMessage);
	
	message->AddInt32("delay", fUpdateDelaySlider->Value());
	message->AddBool("fahrenheit", fFahrenheitBox->Value());
	
	messenger->SendMessage(message);
	delete messenger;
}
