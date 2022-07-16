/*
 * Copyright 2015 Adrián Arroyo Calle <adrian.arroyocalle@gmail.com>
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>
#include <StringView.h>
#include <Window.h>

#include "MainWindow.h"
#include "PreferencesWindow.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PreferencesWindow"


PreferencesWindow::PreferencesWindow(
	BRect frame, MainWindow* parent, int32 updateDelay, DisplayUnit unit)
	:
	BWindow(frame, B_TRANSLATE("Preferences"), B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS
		| B_CLOSE_ON_ESCAPE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	fParent = parent;
	fUpdateDelay = updateDelay;
	fDisplayUnit = unit;

	fCelsiusRadio = new BRadioButton(B_TRANSLATE("Use Celsius °C"), NULL);
	fFahrenheitRadio
		= new BRadioButton(B_TRANSLATE("Use Fahrenheit °F"), NULL);
	fFahrenheitRadio->SetExplicitMinSize(
		BSize(be_plain_font->StringWidth(B_TRANSLATE("Use Fahrenheit °F")) + 50, B_SIZE_UNSET));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGroup(B_VERTICAL)
			.SetInsets(B_USE_WINDOW_SPACING)
			.Add(fCelsiusRadio)
			.Add(fFahrenheitRadio)
			.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.Add(new BButton("ok", B_TRANSLATE("OK"), new BMessage(kSavePrefMessage)))
		.SetInsets(0, 0, 0, B_USE_WINDOW_SPACING)
		.End();

	switch (unit) {
		case CELSIUS:
			fCelsiusRadio->SetValue(1);
			break;
		case FAHRENHEIT:
			fFahrenheitRadio->SetValue(1);
			break;
	}

	CenterIn(frame);
}


void
PreferencesWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kSavePrefMessage:
		{
			_UpdatePreferences();
			QuitRequested();
			Quit();
			break;
		}
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

	if (fCelsiusRadio->Value())
		unit = CELSIUS;
	if (fFahrenheitRadio->Value())
		unit = FAHRENHEIT;

	message->AddInt32("displayUnit", (int32) unit);
	messenger.SendMessage(message);
}


bool
PreferencesWindow::QuitRequested()
{
	BMessenger messenger(fParent);
	BMessage* message = new BMessage(kClosePrefWindowMessage);
	messenger.SendMessage(message);
	return true;
}
