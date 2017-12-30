/*
 * Copyright 2017 Benjamin Amos
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Bitmap.h>
#include <PopUpMenu.h>
#include <StringView.h>
#include <String.h>

#include "ForecastDeskbarView.h"
#include "App.h"

const int pulseRate = 4;
const float pulsesPerSecond = 1000000000 / pulseRate;
float tooltipTimer = 0;

ForecastDeskbarView::ForecastDeskbarView(BRect viewSize, BBitmap* weatherIcon)
	:	BView(viewSize, "ForecastDeskbarView", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED)
{
	fWeatherIcon = weatherIcon;
}

ForecastDeskbarView::ForecastDeskbarView(BMessage* archive)
	:	BView(BRect(0, 0, 15, 15), "ForecastDeskbarView", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED)
{
	//HACK: This is just used to fix the deskbar replicant
	fWeatherIcon = NULL;
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

	return new ForecastDeskbarView(BRect(0, 0, 15, 15), NULL);
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
ForecastDeskbarView::Pulse()
{
	if (tooltipTimer > 0)
	{
		tooltipTimer -= pulseRate;
	}
}

void
ForecastDeskbarView::OnMouseMove(BPoint point)
{
	if (tooltipTimer <= 0)
	{
		tooltipTimer = pulseRate * pulsesPerSecond * 0.5f;

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
