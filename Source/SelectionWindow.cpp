/*
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * Copyright 2014 George White
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Button.h>
#include <GroupLayout.h>
#include <GroupView.h>
#include <ListView.h>
#include <StringView.h>
#include <Url.h>
#include <UrlProtocolRoster.h>
#include <UrlRequest.h>
#include <Window.h>

#include "MainWindow.h"
#include "NetListener.h"
#include "SelectionWindow.h"

SelectionWindow::SelectionWindow(MainWindow* parent,
	BString city, BString cityId)
:
BWindow(BRect(50, 50, 250, 200), "Change location",
	B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS) {
	fParent = parent;
	fCity = city;
	fCityId = cityId;

	BGroupLayout *root = new BGroupLayout(B_VERTICAL);
	this->SetLayout(root);
	
	BGroupView *view = new BGroupView(B_VERTICAL);
	BGroupLayout *layout = view->GroupLayout();
	layout->SetInsets(16);
	this->AddChild(view);
	
	fCityControl = new BTextControl(NULL, "City:", fCity, NULL);
	fIdControl = new BTextControl(NULL, "City ID:", fCityId, NULL);
	
	layout->AddView(fCityControl);
	layout->AddView(new BButton("search", "Search", new BMessage(kSearchMessage)));
	layout->AddView(fIdControl);
	layout->AddView(new BButton("save", "Save", new BMessage(kSaveMessage)));
}


void SelectionWindow::MessageReceived(BMessage *msg) {
	switch (msg->what) {
	case kSearchMessage:
		_FindId();
		break;
	case kDataMessage:
		msg->FindString("id", &fCityId);
		fIdControl->SetText(fCityId);
	case kSaveMessage:
		_UpdateCity();
		break;
	case kFailureMessage:
		fIdControl->SetText("Failed!");
	}
}


void SelectionWindow::_UpdateCity() {
	BMessenger* messenger = new BMessenger(fParent);
	BMessage* message = new BMessage(kUpdateCityMessage);
	
	message->AddString("city", fCityControl->Text());
	message->AddString("id", fIdControl->Text());
	
	messenger->SendMessage(message);
}


void SelectionWindow::_FindId() {
	fIdControl->SetText("Searching...");
	
	BString urlString("https://query.yahooapis.com/v1/public/yql");
	urlString << "?q=select+woeid+from+geo.places(1)+"
		<< "where+text+=\"" << fCityControl->Text() << "\"";
	urlString.ReplaceAll(" ", "+");
	
	BUrlRequest* request =
		BUrlProtocolRoster::MakeRequest(BUrl(urlString.String()),
		new NetListener(this));
	status_t err = request->Run();
}
