/*
 * Copyright 2020 Raheem Idowu <abdurraheemidowu@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _WSMETAWEATHER_H_
#define _WSMETAWEATHER_H_


#include <Looper.h>
#include <String.h>
#include <UrlProtocolListener.h>

enum RequestType {
	CITY_REQUEST,
	WEATHER_REQUEST
};

class WSMetaWeather : public BUrlProtocolListener {
public:
						WSMetaWeather(BHandler* fHandler, RequestType requestType);
	virtual				~WSMetaWeather();
	virtual	void		ResponseStarted(BUrlRequest* caller);
	virtual	void		DataReceived(BUrlRequest* caller, const char* data,
							off_t position, ssize_t size);
	virtual	void		RequestCompleted(BUrlRequest* caller,
							bool success);
private:
			void		_ProcessWeatherData(bool success);
			void		_ProcessCityData(bool success);
			BHandler*	fHandler;
			RequestType fRequestType;
			BMallocIO	fResponseData;
};


#endif // _WSMETAWEATHER_H_
