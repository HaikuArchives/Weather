/*
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _PREFERENCESWINDOW_H_
#define _PREFERENCESWINDOW_H_


#include <CheckBox.h>
#include <Message.h>
#include <Slider.h>
#include <String.h>
#include <Window.h>

class MainWindow;

const int32 kSavePrefMessage = 'SavP';
const int32 kUpdatePrefMessage = 'UpdM';


class PreferencesWindow : public BWindow {
public:
					PreferencesWindow(MainWindow* parent,
						int32 updateDelay, bool cityId);
	
	void			MessageReceived(BMessage *msg);
private:
	void			_UpdatePreferences();

	MainWindow*		fParent;

	int32			fUpdateDelay;
	bool			fFahrenheit;
	
	BSlider*	 	fUpdateDelaySlider;
	BCheckBox*	 	fFahrenheitBox;
};


#endif // _PREFERENCESWINDOW_H_
