/*
 * Copyright 2022 Davide Alfano (Nexus6) <nexus6.haiku@icloud.com>
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

// WMO Weather conditions
//
// 0			Clear sky
// 1, 2, 3		Mainly clear, partly cloudy, and overcast
// 45, 48		Fog and depositing rime fog
// 51, 53, 55	Drizzle: Light, moderate, and dense intensity
// 56, 57		Freezing Drizzle: Light and dense intensity
// 61, 63, 65	Rain: Slight, moderate and heavy intensity
// 66, 67		Freezing Rain: Light and heavy intensity
// 71, 73, 75	Snow fall: Slight, moderate, and heavy intensity
// 77			Snow grains
// 80, 81, 82	Rain showers: Slight, moderate, and violent
// 85, 86		Snow showers slight and heavy
// 95 			*Thunderstorm: Slight or moderate
// 96, 99 		*Thunderstorm with slight and heavy hail

enum weatherConditions {
	WC_CLEAR_SKY = 0,
	WC_MAINLY_CLEAR = 1,
	WC_PARTLY_CLOUDY = 2,
	WC_OVERCAST = 3,
	WC_FOG = 45,
	WC_DEPOSITING_RIME_FOG = 48,
	WC_LIGHT_DRIZZLE = 51,
	WC_MODERATE_DRIZZLE = 53,
	WC_DENSE_DRIZZLE = 55,
	WC_FREEZING_LIGHT_DRIZZLE = 56,
	WC_FREEZING_DENSE_DRIZZLE = 57,
	WC_SLIGHT_RAIN = 61,
	WC_MODERATE_RAIN = 63,
	WC_HEAVY_RAIN = 65,
	WC_LIGHT_FREEZING_RAIN = 66,
	WC_HEAVY_FREEZING_RAIN = 67,
	WC_SLIGHT_SNOW_FALL = 71,
	WC_MODERATE_SNOW_FALL = 73,
	WC_HEAVY_SNOW_FALL = 75,
	WC_SNOW_GRAINS = 77,
	WC_SLIGHT_RAIN_SHOWERS = 80,
	WC_MODERATE_RAIN_SHOWERS = 81,
	WC_HEAVY_RAIN_SHOWERS = 82,
	WC_SLIGHT_SNOW_SHOWERS = 85,
	WC_HEAVY_SNOW_SHOWERS = 86,
	WC_THUNDERSTORM = 95,
	WC_THUNDERSTORM_SLIGHT_HAIL = 96,
	WC_THUNDERSTORM_HEAVY_HAIL = 99
};

//enum weatherConditions {
//	WC_TORNADO,
//	WC_TROPICAL_STORM,
//	WC_HURRICANE,
//	WC_SEVERE_THUNDERSTORM,
//	WC_STORM,
//	WC_MIXED_SNOW_RAIN,
//	WC_SNOW,
//	WC_FREEZING_DRIZZLE,
//	WC_DRIZZE,
//	WC_RAINING,
//	WC_RAINING_SCATTERED,
//	WC_LIGHT_SNOW,
//	WC_FOG,
//	WC_SMOKY,
//	WC_WINDY,
//	WC_COLD,
//	WC_CLOUD,
//	WC_MOSTLY_CLOUDY_NIGHT,
//	WC_MOSTLY_CLOUDY_DAY,
//	WC_NIGHT_FEW_CLOUDS,
//	WC_CLEAR_NIGHT,
//	WC_FEW_CLOUDS,
//	WC_ISOLATED_THUNDERSTORM,
//	WC_SCATTERED_SNOW_SHOWERS,
//	WC_SNOW_SHOWERS,
//	WC_ISOLATED_THUNDERSHOWERS,
//	WC_SHINING,
//	WC_NOT_AVALIABLE,
//};

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
	BBitmap* 		fLightDrizzle[3];
	BBitmap* 		fModerateDenseDrizzle[3];
	BBitmap* 		fFewClouds[3];
	BBitmap* 		fFog[3];
	BBitmap* 		fFreezingDrizzle[3];
	BBitmap* 		fLightSnow[3];
	BBitmap* 		fMixedSnowRain[3];
	BBitmap* 		fMostlyCloudyNight[3];
	BBitmap* 		fNightFewClouds[3];
	BBitmap* 		fRainingScattered[3];
	BBitmap* 		fRaining[3];
	BBitmap* 		fShining[3];
	BBitmap* 		fShiny[3];
	BBitmap* 		fSnow[3];
	BBitmap* 		fStorm[3];
	BBitmap* 		fThunder[3];
	BBitmap* 		fTornado[3];
	BBitmap* 		fTropicalStorm[3];
	BBitmap* 		fCloud[3];
	BBitmap* 		fPartlyCloudy[3];
	BBitmap* 		fIsolatedThunderstorm[3];
	BBitmap* 		fIsolatedThundershowers[3];
	BBitmap* 		fSevereThunderstorm[3];
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
