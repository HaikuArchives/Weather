/*
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * Copyright 2014 George White
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_


#include <Bitmap.h>
#include <Button.h>
#include <GridLayout.h>
#include <GridView.h>
#include <GroupView.h>
#include <MenuBar.h>
#include <Resources.h>
#include <String.h>
#include <StringView.h>
#include <View.h>
#include <Window.h>

#include "SelectionWindow.h"
#include "PreferencesWindow.h"


#define CEL(T) (5.0 / 9.0) * (T - 32.0)
	// Macro converting a Fahrenheit value to a Celsius value

const uint32 kSettingsMessage = 'Pref';
const uint32 kAutoUpdateMessage = 'AutU';
const uint32 kUpdateMessage = 'Upda';
const uint32 kCitySelectionMessage = 'SelC';
const uint32 kOpenPreferencesMessage = 'OPrf';
const uint32 kShowForecastMessage = 'SFor';

extern const char* kSettingsFileName;


class MainWindow : public BWindow {
public:
					MainWindow(void);
	virtual void	MessageReceived(BMessage* msg);
	BMenuBar 		*PrepareMenuBar(void);
	void 			AddView(BView *);
	virtual bool	QuitRequested();
	
private:
	void 			_LoadBitmaps();
	BBitmap* 		_GetWeatherIcon(int32 condition);
	void			_DownloadData(bool forcedForecast = false);
	status_t		_LoadSettings();
	status_t		_SaveSettings();
	void			_ShowForecast(bool);

	BGridView* 		fView;
	BGridLayout* 	fLayout;
	
	BString			fCity;
	BString			fCityId;
	int32			fUpdateDelay;
	bool			fFahrenheit;
	bool			fShowForecast;
	
	int32			fTemperature;
	int32			fCondition;
	BRect			fMainWindowRect;
	SelectionWindow*	fSelectionWindow;
	PreferencesWindow* fPreferencesWindow;
	BMessageRunner*	fAutoUpdate;
	BResources*		fResources;
	
	BBitmap* 		fAlert;
	BBitmap* 		fClearNight;
	BBitmap* 		fClear;
	BBitmap* 		fClouds;
	BBitmap* 		fFewClouds;
	BBitmap* 		fFog;
	BBitmap* 		fNightFewClouds;
	BBitmap* 		fRainingScattered;
	BBitmap* 		fRaining;
	BBitmap* 		fShining;
	BBitmap* 		fShiny;
	BBitmap* 		fSnow;
	BBitmap* 		fStorm;
	BBitmap* 		fThunder;
	BGroupView* 	fForecastView;
	BMenuItem*		fShowForecastMenuItem;
	BButton*		fConditionButton;
	BButton*		fForecastButton[5];
	BStringView*	fConditionView;
	BStringView*	fTemperatureView;
	BStringView*	fCityView;
};

#endif // _MAINWINDOW_H_
