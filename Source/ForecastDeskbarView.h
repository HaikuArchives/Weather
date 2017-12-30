/*
 * Copyright 2017 Benjamin Amos
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef FORECASTDESKBARVIEW_H
#define FORECASTDESKBARVIEW_H

#include <SupportDefs.h>
#include <View.h>
#include <Bitmap.h>

class ForecastDeskbarView : public BView
{
public:
	ForecastDeskbarView(BRect viewSize, BBitmap* weatherIcon);
	virtual void Pulse();
	virtual void OnMouseUp(BPoint point);
	virtual void OnMouseMove(BPoint point);
	virtual void Draw(BRect drawRect);
	void SetWeatherIcon(BBitmap* newIcon);
	virtual status_t	Archive(BMessage* into, bool deep = true) const;
	static BArchivable*	Instantiate(BMessage* archive);
private:
	BBitmap* fWeatherIcon;
};


#endif // FORECASTDESKBARVIEW_H
