/*
 * Copyright 2015 Adrián Arroyo Calle <adrian.arroyocalle@gmail.com>
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <GroupLayout.h>
#include <GroupView.h>
#include <StringView.h>
#include <Window.h>

#include "MainWindow.h"
#include "PreferencesWindow.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PreferencesWindow"

PreferencesWindow::PreferencesWindow(BRect frame, MainWindow* parent,
	int32 updateDelay, DisplayUnit unit)
	:
	BWindow(frame, B_TRANSLATE("Preferences"), B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_CLOSE_ON_ESCAPE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	fParent = parent;
	fUpdateDelay = updateDelay;
	fDisplayUnit = unit;

	BGroupLayout *root = new BGroupLayout(B_VERTICAL);
	this->SetLayout(root);

	BGroupView *view = new BGroupView(B_VERTICAL);
	BGroupLayout *layout = view->GroupLayout();

	static const float spacing = be_control_look->DefaultItemSpacing();
	layout->SetInsets(spacing / 2);
	this->AddChild(view);

	fCelsiusButton = new BRadioButton(B_TRANSLATE("Use degrees Celsius"), NULL);
	fFahrenheitButton = new BRadioButton(B_TRANSLATE("Use degrees Fahrenheit"), NULL);
	fKelvinButton = new BRadioButton(B_TRANSLATE("Use units Kelvin"), NULL);
	fRankineButton = new BRadioButton(B_TRANSLATE("Use degrees Rankine"), NULL);
	fDelisleButton = new BRadioButton(B_TRANSLATE("Use degrees Delisle"), NULL);

	layout->AddView(fCelsiusButton);
	layout->AddView(fFahrenheitButton);
	layout->AddView(fKelvinButton);
	layout->AddView(fRankineButton);
	layout->AddView(fDelisleButton);
	layout->AddView(new BButton("ok", B_TRANSLATE("OK"),
		new BMessage(kSavePrefMessage)));

	switch (unit) {
		case CELSIUS: fCelsiusButton->SetValue(1);break;
		case FAHRENHEIT: fFahrenheitButton->SetValue(1);break;
		case KELVIN: fKelvinButton->SetValue(1);break;
		case RANKINE: fRankineButton->SetValue(1);break;
		case DELISLE: fDelisleButton->SetValue(1);break;
	}
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

	DisplayUnit unit;

	if (fCelsiusButton->Value())
		unit = CELSIUS;
	if (fFahrenheitButton->Value())
		unit = FAHRENHEIT;
	if (fKelvinButton->Value())
		unit = KELVIN;
	if (fRankineButton->Value())
		unit = RANKINE;
	if (fDelisleButton->Value())
		unit = DELISLE;

	message->AddInt32("displayUnit", (int32)unit);

	messenger.SendMessage(message);
}


bool
PreferencesWindow::QuitRequested() {
	BMessenger messenger(fParent);
	BMessage* message = new BMessage(kClosePrefWindowMessage);
	messenger.SendMessage(message);
	return true;
}
