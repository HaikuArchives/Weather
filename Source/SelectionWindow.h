/*
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _SELECTIONWINDOW_H_
#define _SELECTIONWINDOW_H_


#include <Message.h>
#include <String.h>
#include <TextControl.h>
#include <Window.h>

class MainWindow;

const int32 kSearchMessage = 'Srch';
const int32 kSaveMessage = 'Save';
const int32 kUpdateCityMessage = 'Updt';
const int32 kCloseCitySelectionWindowMessage = 'SUCe';


class SelectionWindow : public BWindow {
public:
					SelectionWindow(BRect rect, MainWindow* parent,
						BString city, BString cityId);
	
	virtual void	MessageReceived(BMessage *msg);
	virtual bool	QuitRequested();
private:
	void			_UpdateCity();
	void			_FindId();

	MainWindow*		fParent;

	BString			fCity;
	BString			fCityId;
	
	BTextControl* 	fCityControl;
};


#endif // _SELECTIONWINDOW_H_
