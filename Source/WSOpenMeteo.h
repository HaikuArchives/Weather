/*
 * Copyright 2022 Davide Alfano (Nexus6) <nexus6.haiku@icloud.com>
 * Copyright 2020 Raheem Idowu <abdurraheemidowu@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _WSOPENMETEO_H_
#define _WSOPENMETEO_H_

#include <DataIO.h>
#include <Looper.h>
#include <String.h>
#include <UrlProtocolListener.h>

#include "PreferencesWindow.h"

enum RequestType {
	CITY_REQUEST,
	WEATHER_REQUEST
};

using namespace BPrivate::Network;

class WSOpenMeteo : public BUrlProtocolListener
{
public:
						WSOpenMeteo(const BMessenger& messenger, BMallocIO* responseData, 
							RequestType requestType);
	virtual				~WSOpenMeteo();

	virtual	void		ResponseStarted(BUrlRequest* caller);
	virtual	void		DataReceived(BUrlRequest* caller, const char* data, 
							off_t position, ssize_t size);
	virtual	void		RequestCompleted(BUrlRequest* caller, bool success);

	BString				GetUrl(double longitude, double latitude, DisplayUnit unit);

private:
	void				_ProcessWeatherData(bool success);
	void				_ProcessCityData(bool success);
	BMessenger			fMessenger;
	RequestType 		fRequestType;
	BMallocIO*			fResponseData;
	void				SerializeBMessage(BMessage* message, BString fileName);
};


#endif // _WSOPENMETEO_H_
