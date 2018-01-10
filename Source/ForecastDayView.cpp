/*
	Licensed under the MIT license.
	Made for Haiku.
*/
#include <Screen.h>
#include <String.h>
#include "ForecastDayView.h"
#include "ForecastView.h"

ForecastDayView::ForecastDayView(BRect frame)
	:
	BView(frame, "ForecastDayView", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS),
	fHigh(0),
	fLow(0),
	fIcon(NULL)
{
	fTextColor = ui_color(B_PANEL_TEXT_COLOR);
}


BArchivable*
ForecastDayView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "ForecastView"))
		return NULL;

	return new ForecastDayView(archive);
}


ForecastDayView::ForecastDayView(BMessage* archive)
	:
	BView(archive)
{
	if (archive->FindString("dayLabel", &fDayLabel)!= B_OK)
		fDayLabel = "";

	if (archive->FindInt32("high", &fHigh)!= B_OK)
		fHigh = 0;

	if (archive->FindInt32("low", &fLow)!= B_OK)
		fLow = 0;
}


status_t
ForecastDayView::Archive(BMessage* into, bool deep) const
{
	status_t status;

	status = BView::Archive(into, deep);
	if (status < B_OK)
		return status;

	status = SaveState(into, deep);
	if (status < B_OK)
		return status;

	return B_OK;
}


status_t
ForecastDayView::SaveState(BMessage* into, bool deep) const
{
	status_t status;

	status = into->AddString("dayLabel", fDayLabel);
	if (status != B_OK)
		return status;

	status = into->AddInt32("high", fHigh);
	if (status != B_OK)
		return status;

	status = into->AddInt32("low", fLow);
	if (status != B_OK)
		return status;

	return B_OK;
}


void
ForecastDayView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
}


ForecastDayView::~ForecastDayView(void)
{
}


void
ForecastDayView::FrameResized(float width, float height)
{
	Invalidate();
}


void
ForecastDayView::Draw(BRect urect)
{
	BFont labelFont = be_bold_font;
	font_height finfo;
	labelFont.SetSize(12);
	labelFont.GetHeight(&finfo);
	SetFont(&labelFont);
	rgb_color boxColor;
	rgb_color color = fTextColor;
	if (ViewColor() == B_TRANSPARENT_COLOR) {
		rgb_color low =  BScreen(Window()).DesktopColor();
		if (low.red + low.green + low.blue > 128 * 3) {
			color = tint_color(low, B_DARKEN_MAX_TINT);
			boxColor = make_color(255,255,255);
		} else {
			color = tint_color(low, B_LIGHTEN_MAX_TINT);
			boxColor = make_color(55,55,55);
		}
	}
	BRect boxRect = Bounds();
	// Full Box
	if (ViewColor() == B_TRANSPARENT_COLOR) {
		SetDrawingMode(B_OP_ALPHA);
		boxColor.alpha = 86;
		SetHighColor(boxColor);
	} else {
		SetDrawingMode(B_OP_COPY);
		SetHighColor(tint_color(ViewColor(), 0.7));
	}
	FillRect(Bounds());
	// Header Box
	if (ViewColor() == B_TRANSPARENT_COLOR) {
		SetDrawingMode(B_OP_ALPHA);
		boxColor.alpha = 66;
		SetHighColor(boxColor);
	} else {
		SetDrawingMode(B_OP_COPY);
		SetHighColor(tint_color(ViewColor(), 1.1));
	}

	BRect boxBRect = Bounds();
	boxBRect.bottom = boxBRect.top + finfo.ascent + finfo.descent + finfo.leading + 10;
	FillRect(boxBRect);
	if (ViewColor() == B_TRANSPARENT_COLOR)
		SetDrawingMode(B_OP_ALPHA);
	else
		SetDrawingMode(B_OP_COPY);
	MovePenTo((Bounds().Width() - StringWidth(fDayLabel))/2,
		20 + boxRect.top + (finfo.descent + finfo.leading) - 5) ;
	SetHighColor(color);
	SetLowColor(tint_color(ViewColor(), 1.1));
	DrawString(fDayLabel);

	BFont tempFont = be_plain_font;
	tempFont.SetSize(15);
	tempFont.GetHeight(&finfo);
	SetFont(&tempFont);
	SetLowColor(tint_color(ViewColor(), 0.7));

	BString highString = FormatString(fDisplayUnit,fHigh);

	BString lowString = FormatString(fDisplayUnit,fLow);

	float space = 7;
	if (fDayLabel == "") {
		lowString = highString = "--";
	}

	SetHighColor(color);

	MovePenTo((Bounds().Width() - StringWidth(highString))/2,
		boxRect.bottom - (finfo.descent + finfo.leading + space) * 2 - 5);

	DrawString(highString);

	MovePenTo((Bounds().Width() - StringWidth(lowString))/2,
		boxRect.bottom - (finfo.descent + finfo.leading)  - 5);

	tempFont.SetSize(14);
	SetFont(&tempFont);

	DrawString(lowString);

	if (fIcon) {
		SetDrawingMode(B_OP_OVER);
		float hOffset = (Bounds().Width() - fIcon->Bounds().Width()) / 2;
		float vOffset = (Bounds().Height() -  (finfo.ascent + finfo.descent + finfo.leading) * 2
			- fIcon->Bounds().Height()) / 2;
		vOffset += boxBRect.bottom / 2;
		DrawBitmap(fIcon,
			BPoint(boxRect.left + hOffset, boxRect.top + vOffset));
		SetDrawingMode(B_OP_COPY);
	}
}


void
ForecastDayView::SetIcon(BBitmap *icon)
{
	fIcon = icon;
	Invalidate();
}


void
ForecastDayView::SetDayLabel(BString& dayLabel)
{
	fDayLabel = dayLabel;
	Invalidate();
}


void
ForecastDayView::SetTemp(BString& temp)
{
	fTemp = temp;
	Invalidate();
}


void
ForecastDayView::SetHighTemp(int32 temp)
{
	fHigh = temp;
	Invalidate();
}


void
ForecastDayView::SetLowTemp(int32 temp)
{
	fLow = temp;
	Invalidate();
}


void
ForecastDayView::SetDisplayUnit(DisplayUnit unit)
{
	fDisplayUnit = unit;
	Invalidate();
}


DisplayUnit
ForecastDayView::Unit()
{
	return fDisplayUnit;
}


void
ForecastDayView::SetTextColor(rgb_color color)
{
	fTextColor = color;
	Invalidate();
}
