/*
 * Copyright 2015 Adrián Arroyo Calle <adrian.arroyocalle@gmail.com>
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * Copyright 2014 George White
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _FORECASTVIEW_H_
#define _FORECASTVIEW_H_


#include <Bitmap.h>
#include <Button.h>
#include <DateFormat.h>
#include <Dragger.h>
#include <FormattingConventions.h>
#include <GridLayout.h>
#include <GridView.h>
#include <GroupView.h>
#include <MenuBar.h>
#include <Resources.h>
#include <String.h>
#include <StringView.h>
#include <View.h>
#include <Window.h>

#include "ForecastDayView.h"
#include "LabelView.h"
#include "PreferencesWindow.h"
#include "SelectionWindow.h"

const uint32 kAutoUpdateMessage = 'AutU';
const uint32 kUpdateMessage = 'Upda';
const uint32 kShowForecastMessage = 'SFor';
const uint32 kSettingsMessage = 'Pref';

extern const char* kSettingsFileName;

const uint32 kSizeSmallIcon = 40;
const uint32 kSizeLargeIcon = 80;
const uint32 kSizeDeskBarIcon = 16;

enum weatherIconSize {
	SMALL_ICON,
	LARGE_ICON,
	DESKBAR_ICON
};

class _EXPORT ForecastView;


class ForecastView : public BView {
public:
					ForecastView(BRect frame, BMessage* settings);
					ForecastView(BMessage* archive);
	virtual			~ForecastView();
	virtual void	MessageReceived(BMessage* msg);
	virtual void	AttachedToWindow();
	virtual void	AllAttached();
	virtual void	Draw(BRect updateRect);
virtual status_t	Archive(BMessage* into, bool deep = true) const;
static	BArchivable* Instantiate(BMessage* archive);
status_t			SaveState(BMessage* into, bool deep = true) const;
	void			SetDisplayUnit(DisplayUnit unit);
	void			Reload(bool forcedForecast = false);
	void			StopReload();
	void			SetCityName(BString city);
	BString			CityName();
	void			SetCityId(BString cityId);
	BString			CityId();
	void 			SetCondition(BString condition);
	void			SetUpdateDelay(int32 delay);
	int32			UpdateDelay();
	DisplayUnit		Unit();
	bool			IsFahrenheitDefault();
	void			SetShowForecast(bool showForecast);
	bool			ShowForecast();
	void			SetTextColor(rgb_color color);
	void			SetBackgroundColor(rgb_color color);
	bool			IsDefaultColor() const;
	bool			IsConnected() const;
	BBitmap*		GetWeatherIcon(weatherIconSize size);
	BBitmap* 		GetWeatherIcon(int32 condition, weatherIconSize size);
	int32			GetCondition();
	BString			GetStatus();
	int32			Temperature();
private:
	void			_Init();
	void			_DownloadData();
	static int32	_DownloadDataFunc(void *cookie);
	void 			_LoadBitmaps();
	void			_DeleteBitmaps();
	void			_DeleteIcons(BBitmap* bitmap[2]);
	const char *    _GetWeatherMessage(int32 condition);
	BString			_GetDayText(const BString& day) const;

	status_t		_ApplyState(BMessage *settings);

	void			_ShowForecast(bool);
	void			_LoadIcons(BBitmap*	bitmap[2], uint32 type, const char* name);

	bool			_SupportTransparent();

	bool			_NetworkConnected();

	thread_id		fDownloadThread;
	bool 			fForcedForecast;
	BGridView* 		fView;
	BGridLayout* 	fLayout;
	bool			fReplicated;

	BString			fCity;
	BString			fCityId;
	int32			fUpdateDelay;
	DisplayUnit		fDisplayUnit;
	bool			fShowForecast;

	int32			fTemperature;
	int32			fCondition;

	BDateFormat 	fDateFormat;

	SelectionWindow*	fSelectionWindow;
	PreferencesWindow* fPreferencesWindow;
	BMessageRunner*	fAutoUpdate;
	BMessageRunner*	fDelayUpdateAfterReconnection;
	bool			fConnected;

	BBitmap* 		fAlert[3];
	BBitmap* 		fClearNight[3];
	BBitmap* 		fClear[3];
	BBitmap* 		fClouds[3];
	BBitmap* 		fCold[3];
	BBitmap* 		fDrizzle[3];
	BBitmap* 		fFewClouds[3];
	BBitmap* 		fFog[3];
	BBitmap* 		fFreezingDrizzle[3];
	BBitmap* 		fLightSnow[3];
	BBitmap* 		fMixedSnowRain[3];
	BBitmap* 		fMostlyCloudyNight[3];
	BBitmap* 		fNightFewClouds[3];
	BBitmap* 		fRainingScattered[3];
	BBitmap* 		fRaining[3];
	BBitmap* 		fSevereThunderstorm[3];
	BBitmap* 		fShining[3];
	BBitmap* 		fShiny[3];
	BBitmap* 		fSnow[3];
	BBitmap* 		fStorm[3];
	BBitmap* 		fThunder[3];
	BBitmap* 		fTornado[3];
	BBitmap* 		fTropicalStorm[3];
	BBitmap* 		fCloud[3];
	BBitmap* 		fIsolatedThunderstorm[3];
	BBitmap* 		fIsolatedThundershowers[3];
	BBitmap* 		fHurricane[3];
	BBitmap* 		fScatteredSnowShowers[3];
	BBitmap* 		fSmoky[3];
	BBitmap* 		fSnowShowers[3];
	BBitmap* 		fWindy[3];
	BGroupView*		fInfoView;
	BGroupView*		fNumberView;
	BGroupView* 	fForecastView;
	BMenuItem*		fShowForecastMenuItem;
	BButton*		fConditionButton;
	ForecastDayView*		fForecastDayView[5];
	LabelView*		fConditionView;
	LabelView*		fTemperatureView;
	LabelView*		fCityView;
	BDragger* 		fDragger;
	rgb_color		fBackgroundColor;
	rgb_color		fTextColor;
};

BString FormatString(DisplayUnit unit, int32 temp);

#endif // _FORECASTVIEW_H_
