/*
	Copyright (C) 2014 George White.
	Licensed under the MIT license.
	Made for Haiku.
*/

#include <Window.h>
#include <GroupLayout.h>
#include <GroupView.h>
#include <StringView.h>
#include <ListView.h>
#include <Button.h>
#include <TextControl.h>
#include "SelectionWindow.h"

SelectionWindow::SelectionWindow(void) : BWindow(
								BRect(50, 50, 0, 0),
								"Change location",
								B_TITLED_WINDOW,
								B_ASYNCHRONOUS_CONTROLS |
								B_AUTO_UPDATE_SIZE_LIMITS) {
	BGroupLayout *root = new BGroupLayout(B_VERTICAL);
	this->SetLayout(root);
	
	BGroupView *view = new BGroupView(B_VERTICAL);
	BGroupLayout *layout = view->GroupLayout();
	this->AddChild(view);
	
	layout->AddView(new BStringView("select description", "Select a city to change location:"));
	layout->AddView(new BTextControl(NULL, "search", "Search", NULL));
	layout->AddView(new BListView("list"));
	layout->AddView(new BButton("save", "Save", NULL));
}
	
									
