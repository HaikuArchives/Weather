/*
 * Copyright 2017 Benjamin Amos
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Bitmap.h>
#include <StringView.h>
#include <String.h>
#include <MessageRunner.h>
#include <Looper.h>

#include "ForecastDeskbarView.h"
#include "App.h"

const uint32 kShowToolTipMessage = 'sTTp';
const float kToolTipDelay = 1000000; /*1000000ms = 1s*/

ForecastDeskbarView::ForecastDeskbarView(BRect viewSize, BBitmap* weatherIcon)
	:	BView(viewSize, "ForecastDeskbarView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	fWeatherIcon = weatherIcon;
}

ForecastDeskbarView::ForecastDeskbarView(BMessage* archive)
	:	BView(BRect(0, 0, 15, 15), "ForecastDeskbarView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	//HACK: This is just used to fix the deskbar replicant
	fWeatherIcon = NULL;
}

ForecastDeskbarView::~ForecastDeskbarView()
{
	delete fMessageRunner;
}

void
ForecastDeskbarView::AttachedToWindow()
{
	fMessageRunner = new BMessageRunner(BMessenger(reinterpret_cast<BLooper*>(this)), new BMessage(kShowToolTipMessage), kToolTipDelay, 0);
}

void
ForecastDeskbarView::SetWeatherIcon(BBitmap* newIcon)
{
	fWeatherIcon = newIcon;
}

status_t
ForecastDeskbarView::Archive(BMessage* into, bool deep=true) const
{
	BView::Archive(into, deep);

	return B_OK;
}

BArchivable*
ForecastDeskbarView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "ForecastDeskbarView"))
	{
		return NULL;
	}

	return reinterpret_cast<BArchivable*>(new ForecastDeskbarView(BRect(0, 0, 15, 15), NULL));
}

void
ForecastDeskbarView::Draw(BRect drawRect)
{
	BView::Draw(drawRect);

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	DrawBitmap(fWeatherIcon, BRect(0, 0, 15, 15));
	SetDrawingMode(B_OP_COPY);
}

void
ForecastDeskbarView::MessageReceived(BMessage* message)
{
	if (message->what == kShowToolTipMessage)
	{
		BView* tooltipView = new BView(BRect(0, 0, 20, 30), "tooltipView", B_FOLLOW_ALL, B_WILL_DRAW);
		tooltipView->SetViewColor(255, 255, 255, 255);

		BString weatherDetailsText;
		weatherDetailsText << "Temperature: " << "\n";
		weatherDetailsText << "Conditions: " << "\n";
		weatherDetailsText << "City: " << "\n";
		weatherDetailsText << "Day: " << "\n";
		weatherDetailsText << "Date: " << "\n";
		BStringView* weatherDetails = new BStringView(BRect(0, 0, 20, 30), "weatherDetails", weatherDetailsText.String());

		tooltipView->AddChild(weatherDetails);
		tooltipView->Show();

		fMessageRunner->SetCount(0);
		fSendToolTip = false;
	}
}

void
ForecastDeskbarView::OnMouseMove(BPoint point)
{
	if (!fSendToolTip)
	{
		fMessageRunner->SetCount(1);
		fSendToolTip = true;
	}
}

void
ForecastDeskbarView::OnMouseUp(BPoint point)
{
	uint32 mouseButtonStates = 0;
	if (Window()->CurrentMessage() != NULL)
	{
		mouseButtonStates = Window()->CurrentMessage()->FindInt32("buttons");
	}

	if (mouseButtonStates & B_PRIMARY_MOUSE_BUTTON) //Left click
	{
		App* weatherApp = new App();
		weatherApp->Run();
		delete weatherApp;
		return;
	}
}
