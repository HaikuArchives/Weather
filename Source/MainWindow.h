/*
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * Copyright 2014 George White
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Bitmap.h>
#include <Button.h>
#include <GridLayout.h>
#include <GridView.h>
#include <MenuBar.h>
#include <Resources.h>
#include <String.h>
#include <StringView.h>
#include <View.h>
#include <Window.h>

#define CEL(T) (5.0 / 9.0) * (T - 32.0)
	// Macro converting a Fahrenheit value to a Celsius value


class MainWindow : public BWindow {
private:
	BGridView* 		fView;
	BGridLayout* 	fLayout;
	
	void 			_LoadBitmaps();
	void			_DownloadData(int city);
	
	BString			fCity;
	int				fCityId;
	
	int32			fTemperature;
	int32			fCondition;
	
	BResources*		fResources;
	
	BBitmap* 		fCloudy;
	
	BButton*		fConditionButton;
	BStringView*	fConditionView;
	BStringView*	fTemperatureView;
	BStringView*	fCityView;

public:
					MainWindow(void);
	void 			MessageReceived(BMessage* msg);
	BMenuBar 		*PrepareMenuBar(void);
	void 			AddView(BView *);	
};
