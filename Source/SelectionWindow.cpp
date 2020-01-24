/*
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * Copyright 2014 George White
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Button.h>
#include <Catalog.h>

#include <ControlLook.h>
#include <GroupLayout.h>
#include <GroupView.h>
#include <ListView.h>
#include <StringView.h>
#include <Url.h>
#include <UrlProtocolRoster.h>
#include <UrlRequest.h>
#include <Window.h>

#include "CitiesListSelectionWindow.h"
#include "MainWindow.h"
#include "WSMetaWeather.h"
#include "SelectionWindow.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SelectionWindow"

SelectionWindow::SelectionWindow(BRect rect, MainWindow* parent, BString city,
	BString cityId)
	:
	BWindow(rect, B_TRANSLATE("Change location"), B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS | B_CLOSE_ON_ESCAPE
		| B_AUTO_UPDATE_SIZE_LIMITS),
		fDownloadThread(-1)
 {
	fParent = parent;
	fCity = city;
	fCityId = cityId;

	BGroupLayout *root = new BGroupLayout(B_VERTICAL);
	this->SetLayout(root);
	
	BGroupView *view = new BGroupView(B_HORIZONTAL);
	BGroupLayout *layout = view->GroupLayout();

	static const float spacing = be_control_look->DefaultItemSpacing();
	layout->SetInsets(spacing / 2);
	this->AddChild(view);
	
	fCityControl = new BTextControl(NULL, B_TRANSLATE("City:"), fCity, NULL);
	fCityControl->SetToolTip(B_TRANSLATE("Select city: city, country, region"));

	layout->AddView(fCityControl);
	BButton* button;
	layout->AddView(button = new BButton("search", B_TRANSLATE("OK"),
		new BMessage(kSearchMessage)));
	fCityControl->MakeFocus(true);
	button->MakeDefault(true);
}


void
SelectionWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case kSearchMessage:
		_StartSearch();
		Hide();
		break;
	case kCitiesListMessage: {
		BRect frame(Frame().LeftTop(),BSize(400,200));
		frame.OffsetBy(30,30);
		(new CitiesListSelectionWindow(frame, fParent, msg))->Show();
		break;
		}
	case kDataMessage:
		msg->FindString("id", &fCityId);
		_UpdateCity();
		QuitRequested();
		Close();
		break;
	case kFailureMessage:
		// TODO add a message to the window
		Show();
		break;
	default:
		BWindow::MessageReceived(msg);
	}
}


bool
SelectionWindow::QuitRequested()
{
	BMessenger messenger(fParent);
	BMessage* message = new BMessage(kCloseCitySelectionWindowMessage);
	messenger.SendMessage(message);
	return true;
}


void
SelectionWindow::_UpdateCity()
{
	BMessenger messenger(fParent);
	BMessage* message = new BMessage(kUpdateCityMessage);
	
	message->AddString("city", fCityControl->Text());
	message->AddString("id", fCityId);
	
	messenger.SendMessage(message);
}


void
SelectionWindow::_StartSearch()
{
	_StopSearch();

	fDownloadThread = spawn_thread(&_FindIdFunc,
		"Download Data", B_NORMAL_PRIORITY, this);
	if (fDownloadThread >= 0)
		resume_thread(fDownloadThread);
}


void
SelectionWindow::_StopSearch()
{
	if (fDownloadThread < 0)
		return;
	wait_for_thread(fDownloadThread, NULL);
	fDownloadThread = -1;
}


int32
SelectionWindow::_FindIdFunc(void *cookie)
{
	SelectionWindow* selectionWindow = static_cast<SelectionWindow*>(cookie);
	selectionWindow->_FindId();
	return 0;
}


void
SelectionWindow::_FindId()
{
	BString urlString("https://www.metaweather.com/api/location/search/?query=");
	urlString << fCityControl->Text();
	// Filter out characters that trip up BUrl
	urlString.ReplaceAll(" ", "+");
	urlString.ReplaceAll("<", "");
	urlString.ReplaceAll(">", "");
	urlString.ReplaceAll("\"", "");
	WSMetaWeather listener(this, CITY_REQUEST);

	BUrlRequest* request =
		BUrlProtocolRoster::MakeRequest(BUrl(urlString.String()),
		&listener);

	thread_id thread = request->Run();
	wait_for_thread(thread, NULL);
	delete request;
}
 
