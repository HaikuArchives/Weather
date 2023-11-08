/*
 * Copyright 2018 Benjamin Amos
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Alert.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <IconUtils.h>
#include <Looper.h>
#include <MessageRunner.h>
#include <Roster.h>
#include <String.h>
#include <StringView.h>

#include "App.h"
#include "ForecastDeskbarView.h"
#include "ForecastView.h"

const uint32 kUpdateForecastMessage = 'Updt';
const float kToolTipDelay = 1000000; /*1000000ms = 1s*/

ForecastDeskbarView::ForecastDeskbarView(BRect viewSize)
	:
	BView(viewSize, "ForecastDeskbarView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	// forecastview is only needed to get the weather icon
	fForecastView = new ForecastView(BRect(0, 0, 0, 0));
	AddChild(fForecastView);
	fMessageRunner = NULL;
}


ForecastDeskbarView::ForecastDeskbarView(BMessage* archive)
	:
	BView(archive),
	fMessageRunner(NULL)
{
	// fForecastView is unarchived and already added to the view hierarchy
	fForecastView = static_cast<ForecastView*>(FindView("Weather"));
	entry_ref appRef;
	archive->FindRef("appLocation", &appRef);
	SetAppLocation(appRef);
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

	AdoptParentColors();
}


void
ForecastDeskbarView::Draw(BRect drawRect)
{
	BView::Draw(drawRect);
	SetDrawingMode(B_OP_OVER);
		// TO-DO: Try with 
		// SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	BBitmap* icon = fForecastView->GetWeatherIcon();
	if (icon != NULL)
		DrawBitmap(icon, B_ORIGIN);
	SetDrawingMode(B_OP_COPY);
}


status_t
ForecastDeskbarView::Archive(BMessage* into, bool deep) const
{
	status_t status = BView::Archive(into, deep);
	if (status != B_OK)
		return status;
	
	status = into->AddString("add_on", kSignature);
	if (status != B_OK)
		return status;

	return status;
}


BArchivable*
ForecastDeskbarView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "ForecastDeskbarView"))
		return NULL;

	return new ForecastDeskbarView(archive);
}


void
ForecastDeskbarView::MessageReceived(BMessage* message)
{
	if (message->what == kUpdateForecastMessage) {
		BString weatherDetailsText;
		weatherDetailsText << "Temperature: "
						   << FormatString(fForecastView->Unit(),
								  fForecastView->Temperature())
						   << "\n";
		weatherDetailsText << "Condition: " << fForecastView->GetCondition()
						   << "\n";
		weatherDetailsText << "Location: " << fForecastView->CityName();
		SetToolTip(weatherDetailsText.String());

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


extern "C" BView* instantiate_deskbar_item(float maxWidth, float maxHeight);


BView*
instantiate_deskbar_item(float maxWidth, float maxHeight)
{	
	return new ForecastDeskbarView(BRect(0, 0, maxWidth - 1, maxHeight - 1));
}
