/*
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Message.h>
#include <String.h>
#include <TextControl.h>
#include <Window.h>

class MainWindow;

const int32 kSearchMessage = 'Srch';
const int32 kSaveMessage = 'Save';
const int32 kUpdateCityMessage = 'Updt';


class SelectionWindow : public BWindow {
public:
					SelectionWindow(MainWindow* parent,
						BString city, BString cityId);
	
	void			MessageReceived(BMessage *msg);
private:
	void			_UpdateCity();
	void			_FindId();

	MainWindow*		fParent;

	BString			fCity;
	BString			fCityId;
	
	BTextControl* 	fCityControl;
	BTextControl* 	fIdControl;
};
