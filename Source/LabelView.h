/*
 * Copyright 2015 Adrián Arroyo Calle <adrian.arroyocalle@gmail.com>
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * Copyright 2014 George White
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _LABELVIEW_H_
#define _LABELVIEW_H_


#include <StringView.h>


class LabelView : public BStringView {
public:
			
			LabelView(const char* name, const char* text,
									uint32 flags = B_WILL_DRAW);
			virtual void Draw (BRect updateRect);
};



#endif // _LABELVIEW_H_
