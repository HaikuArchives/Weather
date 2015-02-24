/*
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _NETLISTENER_H_
#define _NETLISTENER_H_


#include <Looper.h>
#include <String.h>
#include <UrlProtocolListener.h>

enum RequestType {
	CITY_REQUEST,
	WEATHER_REQUEST
} ;

const uint32 kDataMessage = 'Data';
const uint32 kForecastDataMessage = 'FDta';
const uint32 kFailureMessage = 'Fail';
const uint32 kUpdateCityName = 'UpCN';
const uint32 kUpdateTTLMessage = 'TTLm';

class NetListener : public BUrlProtocolListener {
public:
						NetListener(BHandler* fHandler, RequestType requestType);
	virtual				~NetListener();
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


#endif // _NETLISTENER_H_
