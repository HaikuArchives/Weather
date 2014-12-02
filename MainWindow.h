/*
	Copyright (C) 2014 George White.
	Licensed under the MIT license.
	Made for Haiku.
*/

#include <Window.h>
#include <GridLayout.h>
#include <GridView.h>
#include <MenuBar.h>
#include <View.h>

class MainWindow : public BWindow {
private:
	BGridView* view;
	BGridLayout* layout;

public:
	MainWindow(void);
	void MessageReceived(BMessage *);
	BMenuBar *PrepareMenuBar(void);
	void AddView(BView *);
};
