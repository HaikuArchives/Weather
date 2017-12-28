/*
 * Copyright 2017 Benjamin Amos
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Bitmap.h>

#include "WeatherDeskbarView.h"
#include "App.h"

WeatherDeskbarView::WeatherDeskbarView(BRect viewSize, BBitmap* weatherIcon)
	:	BView(viewSize, "WeatherDeskbarView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	fWeatherIcon = weatherIcon;
}

void
WeatherDeskbarView::SetWeatherIcon(BBitmap* newIcon)
{
	fWeatherIcon = newIcon;
}

status_t
WeatherDeskbarView::Archive(BMessage* into, bool deep=true) const
{
	BView::Archive(into, deep);

	return B_OK;
}

BArchivable*
WeatherDeskbarView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "DeskbarView"))
	{
		return NULL;
	}

	return new WeatherDeskbarView(BRect(0, 0, 15, 15), NULL);
}

void
WeatherDeskbarView::Draw(BRect drawRect)
{
	BView::Draw(drawRect);

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	DrawBitmap(fWeatherIcon, BRect(0, 0, 15, 15));
	SetDrawingMode(B_OP_COPY);
}

void
WeatherDeskbarView::OnMouseUp()
{
	App* weatherApp = new App();
	weatherApp->Run();
	delete weatherApp;
}
