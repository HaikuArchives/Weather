/*
	Copyright (C) 2014 George White.
	Licensed under the MIT license.
	Made for Haiku.
*/

#include <Window.h>
#include <GridLayout.h>
#include <GridView.h>
#include <View.h>

class MainWindow : public BWindow {
private:
	BGridView* view;
	BGridLayout* layout;

public:
	MainWindow(void);
	void MessageRecieved(BMessage *);
	void AddView(BView *);
};
