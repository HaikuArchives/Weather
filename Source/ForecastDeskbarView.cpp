/*
 * Copyright 2017 Benjamin Amos
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Bitmap.h>
#include <StringView.h>
#include <String.h>
#include <MessageRunner.h>
#include <Looper.h>
#include <Roster.h>

#include "ForecastDeskbarView.h"
#include "ForecastView.h"
#include "App.h"
#include "Util.h"

const uint32 kUpdateForecastMessage = 'Updt';
const float kToolTipDelay = 1000000; /*1000000ms = 1s*/

ForecastDeskbarView::ForecastDeskbarView(BRect viewSize, ForecastView* forecastView)
	:	BView(viewSize, "ForecastDeskbarView", B_FOLLOW_ALL, B_WILL_DRAW | B_NAVIGABLE | B_PULSE_NEEDED)
{
	fForecastView = forecastView;
}

ForecastDeskbarView::ForecastDeskbarView(BMessage* archive)
	:	BView(BRect(0, 0, 15, 15), "ForecastDeskbarView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	fForecastView = NULL;
}

ForecastDeskbarView::~ForecastDeskbarView()
{
	delete fMessageRunner;
}

void
ForecastDeskbarView::AttachedToWindow()
{
	fMessageRunner = new BMessageRunner(BMessenger(this), new BMessage(kUpdateForecastMessage), kToolTipDelay, -1);
	fForecastView->AttachedToWindow();
	fForecastView->Reload(true);

	AddChild(fForecastView);
}

status_t
ForecastDeskbarView::Archive(BMessage* into, bool deep=true) const
{
	BView::Archive(into, deep);
	return B_OK;
}

extern "C" _EXPORT BView*
instantiate_deskbar_item(void)
{
	BMessage settings;
	LoadSettings(settings);
	ForecastDeskbarView* view = new ForecastDeskbarView(BRect(0, 0, 15, 15), new ForecastView(BRect(0, 0, 0, 0), &settings));
	entry_ref appRef;
	settings.FindRef("appLocation", &appRef);
	view->SetAppLocation(appRef);
	return view;
}

BArchivable*
ForecastDeskbarView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "ForecastDeskbarView"))
	{
		return NULL;
	}

	return instantiate_deskbar_item();
}

void
ForecastDeskbarView::Draw(BRect drawRect)
{
	BView::Draw(drawRect);

	SetDrawingMode(B_OP_OVER);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	BBitmap* bitmap = fForecastView->GetWeatherIcon(static_cast<weatherIconSize>(0));
	DrawBitmap(bitmap, BRect(0, 0, 15, 15));
	SetDrawingMode(B_OP_COPY);
}

void
ForecastDeskbarView::MessageReceived(BMessage* message)
{
	if (message->what == kUpdateForecastMessage)
	{
		Draw(BRect(0, 0, 15, 15));
	}
}

void
ForecastDeskbarView::MouseMoved(BPoint point, uint32 message, const BMessage* dragMessage)
{
	BString weatherDetailsText;
	weatherDetailsText << "Temperature: " << FormatString(fForecastView->Unit(), fForecastView->Temperature()) << "\n";
	weatherDetailsText << "Condition: " << fForecastView->GetStatus() << "\n";
	weatherDetailsText << "Location: " << fForecastView->CityName();
	SetToolTip(weatherDetailsText.String());
	ShowToolTip(ToolTip());
}

void
ForecastDeskbarView::MouseDown(BPoint point)
{
	uint32 mouseButtonStates = 0;
	if (Window()->CurrentMessage() != NULL)
	{
		mouseButtonStates = Window()->CurrentMessage()->FindInt32("buttons");
	}

	if (mouseButtonStates & B_PRIMARY_MOUSE_BUTTON) //Left click
	{
		be_roster->Launch(&fAppRef);
	}
}

void ForecastDeskbarView::SetAppLocation(entry_ref location)
{
	fAppRef = location;
}
