/*
 * Copyright 2017 Benjamin Amos
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef WEATHERDESKBARVIEW_H
#define WEATHERDESKBARVIEW_H

#include <SupportDefs.h>
#include <View.h>
#include <Bitmap.h>

class WeatherDeskbarView : public BView
{
public:
	WeatherDeskbarView(BRect viewSize, BBitmap* weatherIcon);
	virtual void OnMouseUp();
	virtual void Draw(BRect drawRect);
	void SetWeatherIcon(BBitmap* newIcon);
	virtual status_t	Archive(BMessage* into, bool deep = true) const;
	static BArchivable*	Instantiate(BMessage* archive);
private:
	BBitmap* fWeatherIcon;
};


#endif // WEATHERDESKBARVIEW_H
