/*
 * Copyright 2015 Adrián Arroyo Calle <adrian.arroyocalle@gmail.com>
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * Copyright 2014 George White
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Screen.h>

#include "LabelView.h"


LabelView::LabelView(const char* name, const char* text, uint32 flags)
	:
	BStringView(name, text, flags)
{
}


void
LabelView::Draw(BRect updateRect)
{
	rgb_color color = HighColor();
	if (ViewColor() == B_TRANSPARENT_COLOR) {
		rgb_color low = BScreen(Window()).DesktopColor();

		if (low.red + low.green + low.blue > 128 * 3)
			color = tint_color(low, B_DARKEN_MAX_TINT);
		else
			color = tint_color(low, B_LIGHTEN_MAX_TINT);
	}
	SetHighColor(color);
	drawing_mode oldMode = DrawingMode();
	SetDrawingMode(B_OP_OVER);
	BStringView::Draw(updateRect);
	SetDrawingMode(oldMode);
}
