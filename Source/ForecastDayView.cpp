/*
	Licensed under the MIT license.
	Made for Haiku.
*/
#include "ForecastDayView.h"

#define CEL(T) (5.0 / 9.0) * (T - 32.0)
	// Macro converting a Fahrenheit value to a Celsius value

ForecastDayView::ForecastDayView(BRect frame)
:BView(frame, "ForecastDayView", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS),
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
	: BView(archive)
{

	if (archive->FindString("dayLabel", &fDayLabel)!= B_OK)
		fDayLabel = "";

	if (archive->FindInt32("high", &fHigh)!= B_OK)
		fHigh = 0;

	if (archive->FindInt32("low", &fLow)!= B_OK)
		fLow = 0;

}

status_t ForecastDayView::Archive(BMessage* into, bool deep) const
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

status_t ForecastDayView::SaveState(BMessage* into, bool deep) const
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
	SetHighColor(tint_color(ViewColor(), 0.7));
	FillRect(Bounds());

	// Header Box
	SetHighColor(tint_color(ViewColor(), 1.1));
	BRect boxBRect = Bounds();
	boxBRect.bottom = boxBRect.top + finfo.ascent + finfo.descent + finfo.leading + 10;
	FillRect(boxBRect);
	MovePenTo((Bounds().Width() - StringWidth(fDayLabel))/2,
		20 + boxRect.top + (finfo.descent + finfo.leading) - 5) ;
	SetHighColor(fTextColor);
	SetLowColor(tint_color(ViewColor(), 1.1));
	DrawString(fDayLabel);

	BFont tempFont = be_plain_font;
	tempFont.SetSize(15);
	tempFont.GetHeight(&finfo);
	SetFont(&tempFont);
	SetLowColor(tint_color(ViewColor(), 0.7));

	BString highString = "";
	if (fFahrenheit)
		highString << fHigh << "°F";
	else
		highString << static_cast<int>(floor(CEL(fHigh))) << "°C";

	BString lowString = "";
	if (fFahrenheit)
		lowString << fLow << "°F";
	else
		lowString << static_cast<int>(floor(CEL(fLow))) << "°C";

	float space = 7;
	if (fDayLabel == "") {
		lowString = highString = "--";
	}

	MovePenTo((Bounds().Width() - StringWidth(highString))/2,
		boxRect.bottom - (finfo.descent + finfo.leading + space) * 2 - 5);

	DrawString(highString);

	MovePenTo((Bounds().Width() - StringWidth(lowString))/2,
		boxRect.bottom - (finfo.descent + finfo.leading)  - 5);

	tempFont.SetSize(14);
	SetFont(&tempFont);
	SetHighColor(tint_color(fTextColor, 0.7));

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

void ForecastDayView::SetHighTemp(int32 temp)
{
	fHigh = temp;
	Invalidate();
}
void ForecastDayView::SetLowTemp(int32 temp)
{
	fLow = temp;
	Invalidate();
}
void ForecastDayView::SetFahrenheit(bool fahrenheit){
	fFahrenheit = fahrenheit;
	Invalidate();
}

bool ForecastDayView::IsFahrenheit(){
	return fFahrenheit;
}

void ForecastDayView::SetTextColor(rgb_color color)
{
	fTextColor = color;
	Invalidate();
}
