/*
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _NETLISTENER_H_
#define _NETLISTENER_H_


#include <String.h>
#include <UrlProtocolListener.h>

class MainWindow;

const int32 kDataMessage = 'Data';

class NetListener : public BUrlProtocolListener {
public:
						NetListener(MainWindow* parent);

	virtual	void		ResponseStarted(BUrlRequest* caller);
	virtual	void		DataReceived(BUrlRequest* caller, const char* data,
							off_t position, ssize_t size);
	virtual	void		RequestCompleted(BUrlRequest* caller,
							bool success);
private:
			MainWindow*	fParent;
			
			BString		fResponse;
};


#endif // _NETLISTENER_H_
