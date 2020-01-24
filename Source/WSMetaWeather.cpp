/*
 * Copyright 2020 Raheem Idowu <abdurraheemidowu@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 * Weather data source using the MetaWeather API at: metaweather.com
 */
#include <Json.h>
#include <Messenger.h>
#include <stdio.h>

#include "WSMetaWeather.h"
#include "MainWindow.h"

WSMetaWeather::WSMetaWeather(BHandler* handler, RequestType requestType)
	:
	BUrlProtocolListener(),
	fHandler(handler),
	fRequestType(requestType)
{
}


WSMetaWeather::~WSMetaWeather()
{
}


void
WSMetaWeather::ResponseStarted(BUrlRequest* caller)
{
}


void
WSMetaWeather::DataReceived(BUrlRequest* caller, const char* data,
	off_t position, ssize_t size) {

	fResponseData.Write(data, size);

}


void
WSMetaWeather::RequestCompleted(BUrlRequest* caller,
	bool success) {

	if (fRequestType == WEATHER_REQUEST)
		_ProcessWeatherData(success);

	if (fRequestType == CITY_REQUEST)
		_ProcessCityData(success);
}


void
WSMetaWeather::_ProcessWeatherData(bool success)
{
	BMessenger messenger(fHandler);
	BString jsonString;

	if (!success) {
		messenger.SendMessage(new BMessage(kFailureMessage));
		return;
	}

	jsonString.SetTo(static_cast<const char*>(fResponseData.Buffer()),
		fResponseData.BufferLength());

	BMessage parsedData;
	BJson parser;

	status_t status = parser.Parse(jsonString, parsedData);
	if (status == B_BAD_DATA) {
		printf("JSON Parser error for data:\n%s\n", jsonString.String());
		BMessage* message = new BMessage(kFailureMessage);
		messenger.SendMessage(message);
		return;
	}
	
	BString detailMessage;
	if (!(parsedData.FindString("detail", &detailMessage) == B_OK)) {		
		//Get name of location
		BString title;
		parsedData.FindString("title", &title);
		BMessage* message = new BMessage(kUpdateCityName);
		message->AddString("city", title);
		messenger.SendMessage(message);
		
		//Get weather forecast
		BMessage weatherData;
		char *name;
		uint32 type;
		int32 count, condition;
		double high, low, temp;
		BString code, text;
		if (parsedData.FindMessage("consolidated_weather", &weatherData) == B_OK) {
			for (int32 i = 0; weatherData.GetInfo(B_MESSAGE_TYPE, i, &name, &type, &count) == B_OK; i++) {
				BMessage forecastDayMessage;
				if (weatherData.FindMessage(name, &forecastDayMessage) == B_OK) {
					forecastDayMessage.FindString("weather_state_abbr", &code);
					forecastDayMessage.FindString("weather_state_name", &text);
					forecastDayMessage.FindDouble("min_temp", &low);
					forecastDayMessage.FindDouble("max_temp", &high);
					forecastDayMessage.FindDouble("the_temp", &temp);

					//Weather assumes farenheit for some reason
					low = (low * (9/5)) + 32;
					high = (high * (9/5)) + 32;
					temp = (temp * (9/5)) + 32;


					if (code == "sn" || code == "h" || code == "sl")
						condition = WC_SNOW;
					if (code == "t")
						condition = WC_STORM;
					if (code == "hr")
						condition = WC_RAINING;
					if (code == "lr")
						condition = WC_DRIZZE;
					if (code == "s")
						condition = WC_RAINING_SCATTERED;
					if (code == "hc")
						condition = WC_MOSTLY_CLOUDY_DAY;
					if (code == "lc")
						condition = WC_CLOUD;
					if (code == "c")
						condition = WC_FEW_CLOUDS;
						
					//Get the weekday
					static int t[] = {0, 3, 2, 5, 0, 3,
						5, 1, 4, 6, 2, 4};
						
					BString date;
					char yearStr[4];
					char monthStr[2];
					char dayStr[2];
					int weekDay;
					
					forecastDayMessage.FindString("applicable_date", &date);
					date.CopyInto(yearStr, 0, 4);
					date.CopyInto(monthStr, 5, 2);
					date.CopyInto(dayStr, 8, 2);
					
					int year, month, day;
					sscanf(yearStr, "%d", &year);
					sscanf(monthStr, "%d", &month);
					sscanf(dayStr, "%d", &day);

					year -= month < 3;
					weekDay = (year + year / 4 - year / 100 + 
						year / 400 + t[month - 1] + day) % 7;					
					
					if (i == 0) {
						BMessage* message = new BMessage(kDataMessage);
						message->AddInt32("temp", (int) temp);
						message->AddInt32("condition", condition);
						message->AddString("text", text);
						messenger.SendMessage(message);
					} else {
						BMessage* message = new BMessage(kForecastDataMessage);
						message->AddInt32("forecast", i-1);
						message->AddInt32("high", (int) high);
						message->AddInt32("low", (int) low);
						message->AddInt32("condition", condition);
						messenger.SendMessage(message);
					}
				}
			}
		}
	}
}

void
WSMetaWeather::_ProcessCityData(bool success)
{
	BMessenger messenger(fHandler);
	BString jsonString;

	if (!success) {
		messenger.SendMessage(new BMessage(kFailureMessage));
		return;
	}

	BMessage parsedData;
	BJson parser;
	jsonString.SetTo(static_cast<const char*>(fResponseData.Buffer()),
		fResponseData.BufferLength());

	status_t status = parser.Parse(jsonString, parsedData);

	if (status == B_BAD_DATA) {
		printf("JSON Parser error for data:\n%s\n", jsonString.String());
		BMessage* message = new BMessage(kFailureMessage);
		messenger.SendMessage(message);
		return;
	}

	char *name;
	uint32 type;
	int32 count;
	double woeid;
	BString locationName;
	BMessage *message = new BMessage(kCitiesListMessage);
	for (int32 i = 0; parsedData.GetInfo(B_MESSAGE_TYPE, i, &name, &type, &count) == B_OK; i++) {
		BMessage locationMessage;
		if (parsedData.FindMessage(name, &locationMessage) == B_OK) {
			locationMessage.FindString("title", &locationName);
			locationMessage.FindDouble("woeid", &woeid);

			char woeidStr[10];
			sprintf(woeidStr, "%.0f", woeid);

			message->AddString("city", locationName);
			message->AddString("woeid", woeidStr);
		}
	}
	messenger.SendMessage(message);
}
