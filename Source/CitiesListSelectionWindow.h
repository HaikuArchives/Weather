/*
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _CITIESLISTSELECTIONWINDOW_H_
#define _CITIESLISTSELECTIONWINDOW_H_


#include <Message.h>
#include <String.h>
#include <TextControl.h>
#include <ListView.h>
#include <Window.h>


const int32 kCloseCityCitiesListSelectionWindowMessage = 'CsSe';
const int32 kSearchMessage = 'Srch';
const int32 kSaveMessage = 'Save';
const int32 kUpdateCityMessage = 'Updt';
const int32 kCloseCitySelectionWindowMessage = 'SUCe';

class CitiesListSelectionWindow : public BWindow
{
public:
					CitiesListSelectionWindow(BRect rect, BWindow* parent,
						BString city, int32 cityId);
	virtual			~CitiesListSelectionWindow();

	virtual void	MessageReceived(BMessage* msg);
	virtual bool	QuitRequested();

private:
	BTextControl*	fCityControl;
	BListView*		fCitiesListView;
	BWindow*		fParent;
	thread_id		fDownloadThread;
	
	void			_StartSearch();
	void			_StopSearch();
	static int32	_FindIdFunc(void *cookie);
	void			_UpdateCity();
	void			_FindId();
	
	BString			fCity;
	BString			fCityFullName;
	int32			fCityId;
};


#endif // _CITIESLISTSELECTIONWINDOW_H_
