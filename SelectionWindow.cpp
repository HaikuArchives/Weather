/*
	Copyright (C) 2014 George White.
	Licensed under the MIT license.
	Made for Haiku.
*/

#include <Window.h>
#include <GroupLayout.h>
#include <GroupView.h>
#include <StringView.h>
#include <Button.h>
#include "SelectorWindow.h"

SelectorWindow::SelectorWindow(void) : BWindow(
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
}
	
									
