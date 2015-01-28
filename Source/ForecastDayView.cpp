#include "ForecastDayView.h"
#include <Debug.h>

ForecastDayView::ForecastDayView(BRect frame)
:BView(frame, "ForecastDayView", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS)
{
}
void ForecastDayView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
}
ForecastDayView::~ForecastDayView(void)
{
}

void ForecastDayView::FrameResized(float width, float height)
{
	Draw(Bounds());
}

void ForecastDayView::Draw(BRect urect)
{
	BFont labelFont = be_bold_font;
	font_height finfo;
	labelFont.SetSize(12);
	labelFont.GetHeight(&finfo);
	SetFont(&labelFont);
	BRect boxRect = Bounds();

	// Full Box
	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), 0.7));
	FillRect(Bounds());

	// Header Box
	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), 1.1));
	BRect boxBRect = Bounds();
	boxBRect.bottom = boxBRect.top + finfo.ascent + finfo.descent + finfo.leading + 10;
	FillRect(boxBRect);
	MovePenTo((Bounds().Width() - StringWidth(fDayLabel))/2,
		20 + boxRect.top + (finfo.descent + finfo.leading) - 5) ;
	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
	SetLowColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), 1.1));      	 
	DrawString(fDayLabel);

	BFont tempFont = be_plain_font;
	tempFont.SetSize(15);
	tempFont.GetHeight(&finfo);
	SetFont(&tempFont);
	SetLowColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), 0.7));
        
	MovePenTo((Bounds().Width() - StringWidth(fTemp))/2,
		boxRect.bottom - (finfo.descent + finfo.leading) - 5);

	DrawString(fTemp);

	if (fIcon) {
		SetDrawingMode(B_OP_OVER);
		float hOffset = (Bounds().Width() - fIcon->Bounds().Width()) / 2;
		float vOffset = (Bounds().Height() -  (finfo.ascent + finfo.descent + finfo.leading)
			- fIcon->Bounds().Height()) / 2;
		vOffset += boxBRect.bottom / 2;
		DrawBitmap(fIcon,
			BPoint(boxRect.left + hOffset, boxRect.top + vOffset));
		SetDrawingMode(B_OP_COPY);
	}
}

void ForecastDayView::SetIcon(BBitmap *icon)
{
	fIcon = icon;
	Invalidate();
}

void ForecastDayView::SetDayLabel(BString& dayLabel)
{
	fDayLabel = dayLabel;
	Invalidate();
}

void ForecastDayView::SetTemp(BString& temp)
{
	fTemp = temp;
	Invalidate();
}
