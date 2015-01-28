#ifndef _FORECASTDAYVIEW_H_
#define _FORECASTDAYVIEW_H_


#include <Bitmap.h>
#include <String.h>
#include <View.h>


class ForecastDayView : public BView
{
public:
					ForecastDayView(BRect);
					~ForecastDayView(void);
	virtual void	FrameResized(float, float);
	virtual void	Draw(BRect);
	virtual void	AttachedToWindow();
			void	SetIcon(BBitmap *icon);
			void	SetDayLabel(BString& dayLabel);
			void	SetTemp(BString& temp);
private:
	BString			fDayLabel;
	BString			fTemp;
	BBitmap*		fIcon;
};

#endif // _FORECASTDAYVIEW_H_
