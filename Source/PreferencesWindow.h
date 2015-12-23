/*
 * Copyright 2015 Adrián Arroyo Calle <adrian.arroyocalle@gmail.com>
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _PREFERENCESWINDOW_H_
#define _PREFERENCESWINDOW_H_


#include <RadioButton.h>
#include <Message.h>
#include <Slider.h>
#include <String.h>
#include <Window.h>

class MainWindow;

const int32 kSavePrefMessage = 'SavP';
const int32 kUpdatePrefMessage = 'UpdM';
const int32 kClosePrefWindowMessage = 'CPrW';

enum DisplayUnit{
	CELSIUS = 1,
	FAHRENHEIT = 2,
	KELVIN = 3,
	RANKINE = 4,
	DELISLE = 5
};
typedef enum DisplayUnit DisplayUnit;

class PreferencesWindow : public BWindow {
public:
					PreferencesWindow(BRect frame, MainWindow* parent,
						int32 updateDelay, DisplayUnit unit);

	void			MessageReceived(BMessage *msg);
	virtual bool	QuitRequested();
private:
	void			_UpdatePreferences();

	MainWindow*		fParent;

	int32			fUpdateDelay;
	DisplayUnit		fDisplayUnit;

	BRadioButton*	fCelsiusButton;
	BRadioButton*	fFahrenheitButton;
	BRadioButton*	fKelvinButton;
	BRadioButton*	fRankineButton;
	BRadioButton*	fDelisleButton;
};


#endif // _PREFERENCESWINDOW_H_
