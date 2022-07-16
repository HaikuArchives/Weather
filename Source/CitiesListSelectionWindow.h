/*
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _CITIESLISTSELECTIONWINDOW_H_
#define _CITIESLISTSELECTIONWINDOW_H_


#include <Message.h>
#include <String.h>
#include <Window.h>


const int32 kCloseCityCitiesListSelectionWindowMessage = 'CsSe';


class CitiesListSelectionWindow : public BWindow
{
public:
					CitiesListSelectionWindow(BRect rect, BWindow* parent,
						BMessage* citiesMessage);
	virtual			~CitiesListSelectionWindow();

	virtual void	MessageReceived(BMessage* msg);
	virtual bool	QuitRequested();

private:
	BListView*		fCitiesListView;
	BWindow*		fParent;
};


#endif // _CITIESLISTSELECTIONWINDOW_H_
