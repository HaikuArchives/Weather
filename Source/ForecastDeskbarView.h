/*
 * Copyright 2018 Benjamin Amos
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef FORECASTDESKBARVIEW_H
#define FORECASTDESKBARVIEW_H

#include <SupportDefs.h>
#include <View.h>
#include <Bitmap.h>
#include <MessageRunner.h>
#include <Looper.h>
#include <Entry.h>

#include "ForecastView.h"

class ForecastDeskbarView : public BView
{
public:
	ForecastDeskbarView(BRect viewSize, ForecastView* forecastView);
	~ForecastDeskbarView();
	virtual void AttachedToWindow();
	virtual void MouseDown(BPoint point);
	virtual void MouseMoved(BPoint point, uint32 message, const BMessage* dragMessage);
	virtual void Draw(BRect drawRect);
	virtual void MessageReceived(BMessage* message);
	static BArchivable*	Instantiate(BMessage* archive);
	void SetAppLocation(entry_ref location);
private:
	ForecastView* fForecastView;
	BMessageRunner* fMessageRunner;
	entry_ref fAppRef;
};


#endif // FORECASTDESKBARVIEW_H
