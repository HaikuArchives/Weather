/*
 * Copyright 2018 Benjamin Amos
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Bitmap.h>
#include <StringView.h>
#include <String.h>
#include <MessageRunner.h>
#include <Looper.h>
#include <Roster.h>
#include <Alert.h>

#include "ForecastDeskbarView.h"
#include "ForecastView.h"
#include "App.h"
#include "Util.h"

const uint32 kUpdateForecastMessage = 'Updt';
const float kToolTipDelay = 1000000; /*1000000ms = 1s*/

ForecastDeskbarView::ForecastDeskbarView(BRect viewSize, ForecastView* forecastView)
	:	BView(viewSize, "ForecastDeskbarView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	fForecastView = forecastView;
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
	fMessageRunner = new BMessageRunner(BMessenger(this), new BMessage(kUpdateForecastMessage), kToolTipDelay, -1);

	fForecastView->SetShowForecast(true);
	AddChild(fForecastView);

	AdoptParentColors();
}

extern "C" _EXPORT BView* instantiate_deskbar_item();
BView* instantiate_deskbar_item()
{
	BMessage settings;
	LoadSettings(settings);
	ForecastDeskbarView* view = new ForecastDeskbarView(BRect(0, 0, 16, 16), new ForecastView(BRect(0, 0, 0, 0), &settings));
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
	//SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	BBitmap* bitmap = fForecastView->GetWeatherIcon(static_cast<weatherIconSize>(1));
	if (bitmap)
		DrawBitmapAsync(bitmap, BPoint(0,0));
	SetDrawingMode(B_OP_COPY);
}

BArchivable* ForecastDeskbarView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "ForecastDeskbarView"))
	{
		return NULL;
	}

	return reinterpret_cast<BArchivable*>(instantiate_deskbar_item());
}

void
ForecastDeskbarView::MessageReceived(BMessage* message)
{
	if (message->what == kUpdateForecastMessage)
	{
		BString weatherDetailsText;
		weatherDetailsText << "Temperature: " << FormatString(fForecastView->Unit(), fForecastView->Temperature()) << "\n";
		weatherDetailsText << "Condition: " << fForecastView->GetCondition() << "\n";
		weatherDetailsText << "Location: " << fForecastView->CityName();
		SetToolTip(weatherDetailsText.String());

		Invalidate();
	}
	else
	{
		BView::MessageReceived(message);
	}
}

void
ForecastDeskbarView::MouseMoved(BPoint point, uint32 message, const BMessage* dragMessage)
{
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
