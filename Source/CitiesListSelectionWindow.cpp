/*
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Button.h>
#include <Catalog.h>

#include <ControlLook.h>
#include <GroupLayout.h>
#include <GroupView.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <ScrollView.h>
#include <StringView.h>
#include <Url.h>
#include <UrlProtocolRoster.h>
#include <UrlRequest.h>
#include <Window.h>

#include "MainWindow.h"
#include "NetListener.h"
#include "CitiesListSelectionWindow.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CitiesListSelectionWindow"


class CityItem : public BStringItem {
public:
								CityItem(const char* text,
									const char* woeid, const char* countryCode);
	virtual						~CityItem();
								BString Woeid() const {return fWoeid; };
	virtual	void				DrawItem(BView* owner, BRect frame,
									bool complete = false);
	virtual	void				Update(BView* owner, const BFont* font);
private:
			BString				fWoeid;
			BString				fCountryCode;
			BBitmap*			fIcon;
};


CityItem::CityItem(const char* text, const char* woeid, const char* countryCode):
	BStringItem(text),
	fWoeid(woeid),
	fCountryCode(countryCode),
	fIcon(NULL)
{
};

CityItem::~CityItem()
{
	delete fIcon;
}

void
CityItem::DrawItem(BView* owner, BRect frame,
	bool complete)
{
	float iconSize = 0;

	if (fIcon != NULL && fIcon->IsValid())
		iconSize = fIcon->Bounds().Width();

	BRect offsetFrame(frame);
	offsetFrame.left += iconSize + be_control_look->DefaultLabelSpacing();

	BStringItem::DrawItem(owner, offsetFrame, complete);

	if (fIcon != NULL && fIcon->IsValid())
	{
		BRect iconFrame(frame.left + be_control_look->DefaultLabelSpacing(),
			frame.top,
			frame.left + iconSize - 1 + be_control_look->DefaultLabelSpacing(),
			frame.top + iconSize - 1);
		if (IsSelected() ) {
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

	if (fCountryCode.IsEmpty())
		return;

	fIcon = new(std::nothrow) BBitmap(BRect(0, 0, iconSize - 1, iconSize - 1),
		B_RGBA32);
	if (fIcon != NULL && BLocaleRoster::Default()->GetFlagIconForCountry(fIcon,
			fCountryCode.String()) != B_OK) {
		delete fIcon;
		fIcon = NULL;
	}
}


const uint32 kSelectedCity = 'SeCy';
const uint32 kCancelCity = 'CncC';
const uint32 kChangeSelectedCity = 'CeCy';

CitiesListSelectionWindow::CitiesListSelectionWindow(BRect rect, BWindow* parent,
	BMessage* citiesMessage)
	:
	BWindow(rect, B_TRANSLATE("Choose location"), B_MODAL_WINDOW,
		B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS | B_CLOSE_ON_ESCAPE
		| B_AUTO_UPDATE_SIZE_LIMITS)
 {
	fParent = parent;
	fCitiesListView = new BListView("citiesList");
	BScrollView* fCitiesListSV = new BScrollView("citiesList", fCitiesListView,
		0, false, true);
	fCitiesListView->SetInvocationMessage(new BMessage(kSelectedCity));
	fCitiesListView->SetSelectionMessage(new BMessage(kChangeSelectedCity));
	int32 index = 0;
	BString completeCity, woeid;
	while(citiesMessage->FindString("city", index, &completeCity) == B_OK
		&& citiesMessage->FindString("woeid", index, &woeid) == B_OK )
	{
		BString countryCode;
		citiesMessage->FindString("countryCode", index, &countryCode);
		fCitiesListView->AddItem(new CityItem(completeCity, woeid, countryCode));
		index++;
	}
	fCitiesListView->Select(0);
	BButton* fButtonOk = new BButton("ok", B_TRANSLATE("Ok"),
		new BMessage(kSelectedCity));
	BButton* fButtonCancel = new BButton("cancel", B_TRANSLATE("Cancel"),
		new BMessage(kCancelCity));
	fCitiesListView->ResizeToPreferred();
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(fCitiesListSV)
		.AddGroup(B_HORIZONTAL)
				.AddGlue()
				.Add(fButtonCancel)
				.Add(fButtonOk)
		.End()
	.End();
	fButtonOk->MakeDefault(true);
}


CitiesListSelectionWindow::~CitiesListSelectionWindow()
{
	BListItem* cityItem;
	for(int32 index = 0; cityItem = fCitiesListView->ItemAt(index); index++)
		delete cityItem;
}

void
CitiesListSelectionWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case kSelectedCity: {
		BMessenger messenger(fParent);
		int32 selected = fCitiesListView->CurrentSelection();
		if (selected < 0)
			return;
		BMessage* message = new BMessage(kUpdateCityMessage);
		CityItem* cityItem = dynamic_cast<CityItem*>(fCitiesListView->ItemAt(selected));
		message->AddString("city", cityItem->Text());
		message->AddString("id", cityItem->Woeid());
		messenger.SendMessage(message);
		Quit();
		}
		break;
	case kCancelCity: {
		BMessenger messenger(fParent);
		BMessage* message = new BMessage(kCitySelectionMessage);
		messenger.SendMessage(message);
		Quit();
		}
		break;
	case kChangeSelectedCity: {
		int32 selected = fCitiesListView->CurrentSelection();
		if (selected < 0)
			fCitiesListView->Select(fCitiesListView->CountItems()-1);
		}
		break;
	default:
		BWindow::MessageReceived(msg);
	}
}


bool
CitiesListSelectionWindow::QuitRequested()
{
	BMessenger messenger(fParent);
	BMessage* message = new BMessage(kCloseCityCitiesListSelectionWindowMessage);
	messenger.SendMessage(message);
	return true;
}


