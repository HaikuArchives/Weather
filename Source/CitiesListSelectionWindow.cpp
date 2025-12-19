/*
 * Copyright 2022 Davide Alfano (Nexus6) <nexus6.haiku@icloud.com>
 * Et al.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Button.h>
#include <Catalog.h>

#include <ControlLook.h>
#include <GroupLayout.h>
#include <GroupView.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <Locale.h>
#include <ScrollView.h>
#include <StringView.h>
#include <Url.h>
#include <UrlProtocolRoster.h>
#include <UrlRequest.h>
#include <Window.h>

#include <memory>

#include "CitiesListSelectionWindow.h"
#include "MainWindow.h"
#include "WSOpenMeteo.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CitiesListSelectionWindow"


class CityItem : public BStringItem
{

	public:
								CityItem(const int32 id, const char* city, 
									const char* country, const char* extendedInfo);
		virtual 				~CityItem();

		virtual void 			DrawItem(BView* owner, BRect frame, bool complete = false);
		virtual void 			Update(BView* owner, const BFont* font);

		int32 					Id;
		double 					Longitude;
		double 					Latitude;
		int32 					CountryId;
		BString 				City;
		BString 				DisplayName;
		BString 				Country;
		BString 				ExtendedInfo;

	private:
		BBitmap* 				fIcon;
};


CityItem::CityItem(const int32 id, const char* city, const char* country, const char* extendedInfo)
	:
	BStringItem(city),
	Id(id),
	City(city),
	ExtendedInfo(extendedInfo),
	Country(country),
	fIcon(NULL)
{
	DisplayName << ExtendedInfo;
	this->SetText(DisplayName);
};


CityItem::~CityItem()
{
	delete fIcon;
}


void
CityItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	float iconSize = 0;

	if (fIcon != NULL && fIcon->IsValid())
		iconSize = fIcon->Bounds().Width();

	BRect offsetFrame(frame);
	offsetFrame.left += iconSize + be_control_look->DefaultLabelSpacing();

	BStringItem::DrawItem(owner, offsetFrame, complete);

	if (fIcon != NULL && fIcon->IsValid()) {
		BRect iconFrame(frame.left + be_control_look->DefaultLabelSpacing(), frame.top,
			frame.left + iconSize - 1 + be_control_look->DefaultLabelSpacing(),
			frame.top + iconSize - 1);
		if (IsSelected()) {
			owner->SetHighColor(ui_color(B_LIST_SELECTED_BACKGROUND_COLOR));
			frame.right = offsetFrame.left;
			owner->FillRect(frame);
		}
		owner->SetDrawingMode(B_OP_OVER);
		owner->DrawBitmap(fIcon, iconFrame);
		owner->SetDrawingMode(B_OP_COPY);
	}
};


void
CityItem::Update(BView* owner, const BFont* font)
{
	BStringItem::Update(owner, font);

	float iconSize = Height();
	SetWidth(Width() + iconSize + be_control_look->DefaultLabelSpacing());

	BString countryIdString;
	countryIdString << CountryId;

	fIcon = new (std::nothrow)
		BBitmap(BRect(0, 0, iconSize - 1, iconSize - 1), B_RGBA32);
	if (fIcon != NULL
		&& BLocaleRoster::Default()->GetFlagIconForCountry(
			   fIcon, countryIdString.String())
			!= B_OK) {
		delete fIcon;
		fIcon = NULL;
	}
}


const uint32 kSelectedCity = 'SeCy';
const uint32 kCancelCity = 'CncC';


CitiesListSelectionWindow::CitiesListSelectionWindow(BRect rect, BWindow* parent, BString city,
	int32 cityId)
	:
	BWindow(rect, B_TRANSLATE("Choose location"), B_TITLED_WINDOW, B_NOT_ZOOMABLE
		| B_ASYNCHRONOUS_CONTROLS | B_CLOSE_ON_ESCAPE | B_AUTO_UPDATE_SIZE_LIMITS),
	fDownloadThread(-1)
{
	fParent = parent;
	fCitiesListView = new BListView("citiesList");
	BScrollView* fCitiesListSV
		= new BScrollView("citiesList", fCitiesListView, 0, false, true);
	fCitiesListView->SetInvocationMessage(new BMessage(kSelectedCity));
	fCityId = cityId;
	fCitiesListView->Select(0);

	fCityControl = new BTextControl(NULL, B_TRANSLATE("City:"), fCity, NULL);
	fCityControl->SetToolTip(B_TRANSLATE("Enter location: city, country, region"));
	fCityControl->SetModificationMessage(
		new BMessage(kSearchMessage));

	BButton* fButtonOk
		= new BButton("ok", B_TRANSLATE("OK"), new BMessage(kSelectedCity));
	BButton* fButtonCancel = new BButton(
		"cancel", B_TRANSLATE("Cancel"), new BMessage(kCancelCity));

	fCitiesListView->ResizeToPreferred();
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(fCityControl)
		.Add(fCitiesListSV)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fButtonCancel)
			.Add(fButtonOk)
			.End()
		.End();
	fButtonOk->MakeDefault(true);
	fCityControl->MakeFocus(true);

	_StartSearch();
}


CitiesListSelectionWindow::~CitiesListSelectionWindow()
{
	BListItem* cityItem;
	for (int32 index = 0; cityItem = fCitiesListView->ItemAt(index); index++)
		delete cityItem;
}


void
CitiesListSelectionWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kCitiesListMessage:
		{
			fCitiesListView->MakeEmpty();
			int index = 0;
			while (msg->FindString("city", index, &fCity) == B_OK) {
				BString city;
				double latitude = 0L;
				double longitude = 0L;
				int32 countryId = 0L;
				int32 id = 0L;
				BString country = "";
				BString extendedInfo = "";

				msg->FindInt32("id", index, &id);
				msg->FindString("city", index, &city);
				msg->FindString("country", index, &country);
				msg->FindInt32("country_id", index, &countryId);
				msg->FindString("extended_info", index, &extendedInfo);
				msg->FindDouble("longitude", index, &longitude);
				msg->FindDouble("latitude", index, &latitude);

				CityItem* cityItem = new CityItem(id, city, country, extendedInfo);
				cityItem->Latitude = latitude;
				cityItem->Longitude = longitude;
				cityItem->CountryId = countryId;

				fCitiesListView->AddItem(cityItem);
				index++;
			}
			fCitiesListView->Select(0);
			break;
		}
		case kSearchMessage:
		{
			_StartSearch();
			break;
		}
		case kDataMessage:
		{
			msg->FindInt32("id", &fCityId);
			_UpdateCity();
			QuitRequested();
			Close();
			break;
		}
		case kFailureMessage:
		{
			// TODO add a message to the window
			break;
		}
		case kSelectedCity:
		{
			BMessenger messenger(fParent);
			int32 selected = fCitiesListView->CurrentSelection();
			if (selected < 0)
				return;
			BMessage* message = new BMessage(kUpdateCityMessage);
			CityItem* cityItem
				= dynamic_cast<CityItem*>(fCitiesListView->ItemAt(selected));
			message->AddString("city", cityItem->Text());
			message->AddInt32("id", cityItem->Id);
			message->AddString("country", cityItem->Country);
			message->AddInt32("country_id", cityItem->CountryId);
			message->AddString("extended_info", cityItem->ExtendedInfo);
			message->AddDouble("longitude", cityItem->Longitude);
			message->AddDouble("latitude", cityItem->Latitude);
			messenger.SendMessage(message);
			QuitRequested();
			Close();
			break;
		}
		case kCancelCity:
		{
			QuitRequested();
			Close();
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}

void
CitiesListSelectionWindow::_UpdateCity()
{
	BMessenger messenger(fParent);
	BMessage* message = new BMessage(kUpdateCityMessage);

	message->AddString("city", fCityControl->Text());
	message->AddInt32("id", fCityId);

	messenger.SendMessage(message);
}

void
CitiesListSelectionWindow::_StartSearch()
{
	_StopSearch();

	fDownloadThread
		= spawn_thread(&_FindIdFunc, "Download Data", B_NORMAL_PRIORITY, this);
	if (fDownloadThread >= 0)
		resume_thread(fDownloadThread);
}

int32
CitiesListSelectionWindow::_FindIdFunc(void* cookie)
{
	CitiesListSelectionWindow* selectionWindow = static_cast<CitiesListSelectionWindow*>(cookie);
	selectionWindow->_FindId();
	return 0;
}

void
CitiesListSelectionWindow::_StopSearch()
{
	if (fDownloadThread < 0)
		return;
	wait_for_thread(fDownloadThread, NULL);
	fDownloadThread = -1;
}

void
CitiesListSelectionWindow::_FindId()
{
	BString urlString("https://geocoding-api.open-meteo.com/v1/search?name=");
	urlString << fCityControl->Text();

	// use translated queries and results in local language if available
	// otherwise return english or the native location name. Lower-cased.
	BFormattingConventions conventions;
	if (BLocale::Default()->GetFormattingConventions(&conventions) == B_OK
		&& conventions.LanguageCode() != NULL)
	{
		BString languageCode = conventions.LanguageCode();
		urlString << "&language=" << languageCode.ToLower();
	}

	// Filter out characters that trip up BUrl
	urlString.ReplaceAll(" ", "+");
	urlString.ReplaceAll("<", "");
	urlString.ReplaceAll(">", "");
	urlString.ReplaceAll("\"", "");

	BMallocIO requestData;
	WSOpenMeteo listener(this, &requestData, CITY_REQUEST);

#if B_HAIKU_VERSION < B_HAIKU_R1_PRE_BETA_6
	BUrl url(urlString.String());
#else
	BUrl url(urlString.String(), true);
#endif
	BUrlRequest* request
		= BUrlProtocolRoster::MakeRequest(url, &requestData, &listener);

	thread_id thread = request->Run();
	wait_for_thread(thread, NULL);
	delete request;
}

bool
CitiesListSelectionWindow::QuitRequested()
{
	BMessenger messenger(fParent);
	BMessage* message = new BMessage(kCloseCitySelectionWindowMessage);
	messenger.SendMessage(message);
	return true;
}
