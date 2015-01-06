/*
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * Copyright 2014 George White
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Bitmap.h>
#include <GridLayout.h>
#include <GridView.h>
#include <MenuBar.h>
#include <View.h>
#include <Window.h>


class MainWindow : public BWindow {
private:
	BGridView* 		view;
	BGridLayout* 	layout;
	
	void 			_LoadBitmaps();
	
	BBitmap* 		fCloudy;

public:
					MainWindow(void);
	void 			MessageReceived(BMessage *);
	BMenuBar 		*PrepareMenuBar(void);
	void 			AddView(BView *);
};
