/*
 * Copyright 2022 Nexus6 (Davide Alfano) <nexus6.haiku@icloud.com>
 * Copyright 2020 Raheem Idowu <abdurraheemidowu@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _WSOPENMETEO_H_
#define _WSOPENMETEO_H_


#include <Looper.h>
#include <String.h>
#include <UrlProtocolListener.h>

enum RequestType {
	CITY_REQUEST,
	WEATHER_REQUEST
};

class WSOpenMeteo : public BUrlProtocolListener {
public:
						WSOpenMeteo(BHandler* fHandler, RequestType requestType);
	virtual				~WSOpenMeteo();
	virtual	void		ResponseStarted(BUrlRequest* caller);
	virtual	void		DataReceived(BUrlRequest* caller, const char* data,
							off_t position, ssize_t size);
	virtual	void		RequestCompleted(BUrlRequest* caller,
							bool success);
	
	BString				GetUrl(BString cityId);
	
private:
			void		_ProcessWeatherData(bool success);
			void		_ProcessCityData(bool success);
			BHandler*	fHandler;
			RequestType fRequestType;
			BMallocIO	fResponseData;
};


#endif // _WSOPENMETEO_H_
