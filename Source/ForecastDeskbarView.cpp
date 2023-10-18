/*
 * Copyright 2018 Benjamin Amos
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Bitmap.h>
#include <Catalog.h>
#include <Looper.h>
#include <MessageRunner.h>
#include <Roster.h>
#include <String.h>
#include <StringView.h>

#include "ForecastDeskbarView.h"
#include "ForecastView.h"
#include "Util.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ForecastDeskbarView"

const uint32 kUpdateForecastMessage = 'Updt';
const float kToolTipDelay = 1000000; /*1000000ms = 1s*/
int fMaxHeight;

extern "C" _EXPORT BView* instantiate_deskbar_item(float maxWidth, float maxHeight);

ForecastDeskbarView::ForecastDeskbarView(BRect viewSize, ForecastView* forecastView)
	:
	BView(viewSize, "ForecastDeskbarView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	fForecastView = forecastView;
	fMessageRunner = NULL;
}

ForecastDeskbarView::ForecastDeskbarView(BMessage* archive)
	:
	BView(archive)
{
	fMessageRunner = NULL;
}


ForecastDeskbarView::~ForecastDeskbarView()
{
	delete fMessageRunner;
	delete fForecastView;
}


void
ForecastDeskbarView::AttachedToWindow()
{
	fMessageRunner = new BMessageRunner(BMessenger(this),
		new BMessage(kUpdateForecastMessage), kToolTipDelay, -1);

	fForecastView->SetShowForecast(true);
	AddChild(fForecastView);

	AdoptParentColors();
}


extern "C" _EXPORT BView*
instantiate_deskbar_item(float maxWidth, float maxHeight)
{
	fMaxHeight = maxHeight;
	BMessage settings;
	LoadSettings(settings);
	ForecastDeskbarView* view = new ForecastDeskbarView(
		BRect(0, 0, maxHeight, maxHeight),
		new ForecastView(BRect(0, 0, 0, 0),
		&settings));
	entry_ref appRef;
	settings.FindRef("appLocation", &appRef);
	view->SetAppLocation(appRef);
	return view;
}


void
ForecastDeskbarView::Draw(BRect drawRect)
{
	BView::Draw(drawRect);

	SetDrawingMode(B_OP_OVER);
		// TO-DO: Try with
		// SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	BBitmap* bitmap = fForecastView->GetWeatherIcon(static_cast<weatherIconSize>(1));
	if (bitmap)
		DrawBitmapAsync(bitmap, drawRect);
	SetDrawingMode(B_OP_COPY);
}


BArchivable*
ForecastDeskbarView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "ForecastDeskbarView"))
		return NULL;

	return reinterpret_cast<BArchivable*>(instantiate_deskbar_item(fMaxHeight, fMaxHeight));
}


void
ForecastDeskbarView::MessageReceived(BMessage* message)
{
	if (message->what == kUpdateForecastMessage) {
		BString tooltip;
		BString temperature = FormatString(fForecastView->Unit(), fForecastView->Temperature());
		tooltip.SetToFormat(B_TRANSLATE("Temperature: %s\nCondition: %s\nLocation: %s"),
			temperature.String(),
			fForecastView->GetStatus().String(),
			fForecastView->CityName().String());
		SetToolTip(tooltip);
		Invalidate();
	} else
		BView::MessageReceived(message);
}


void
ForecastDeskbarView::MouseMoved(
	BPoint point, uint32 message, const BMessage* dragMessage)
{
	ShowToolTip(ToolTip());
}


void
ForecastDeskbarView::MouseDown(BPoint point)
{
	uint32 mouseButtonStates = 0;
	if (Window()->CurrentMessage() != NULL)
		mouseButtonStates = Window()->CurrentMessage()->FindInt32("buttons");

	if (mouseButtonStates & B_PRIMARY_MOUSE_BUTTON) // Left click
		be_roster->Launch(&fAppRef);
}


void
ForecastDeskbarView::SetAppLocation(entry_ref location)
{
	fAppRef = location;
}
