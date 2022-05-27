/*
 * Copyright 2022 Davide Alfano (Nexus6) <nexus6.haiku@icloud.com>
 * Copyright 2020 Raheem Idowu <abdurraheemidowu@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 * Weather data source using the MetaWeather API at: metaweather.com
 */
#include <Json.h>
#include <Messenger.h>
#include <stdio.h>

// remove
#include <StorageKit.h>
#include <Alert.h>

#include "WSOpenMeteo.h"
#include "MainWindow.h"

WSOpenMeteo::WSOpenMeteo(BHandler* handler, RequestType requestType)
	:
	BUrlProtocolListener(),
	fHandler(handler),
	fRequestType(requestType)
{
}


WSOpenMeteo::~WSOpenMeteo()
{
}


void
WSOpenMeteo::ResponseStarted(BUrlRequest* caller)
{
}


void
WSOpenMeteo::DataReceived(BUrlRequest* caller, const char* data,
	off_t position, ssize_t size) {

	fResponseData.Write(data, size);

}


void
WSOpenMeteo::RequestCompleted(BUrlRequest* caller,
	bool success) {

	if (fRequestType == WEATHER_REQUEST)
		_ProcessWeatherData(success);

	if (fRequestType == CITY_REQUEST)
		_ProcessCityData(success);
}

BString
WSOpenMeteo::GetUrl(BString cityId) {
	//BString urlString("https://www.openmeteo.com/api/location/");
	//urlString << fCityId << "/";
	BString urlString("https://api.open-meteo.com/v1/forecast?latitude=45.49624&longitude=9.29323&daily=weathercode,temperature_2m_max,temperature_2m_min&current_weather=true&timezone=Europe%2FBerlin");
	return urlString;
}

void
WSOpenMeteo::_ProcessWeatherData(bool success)
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
	
// pragma remove
// temp - save parsed data to file
//	BPath p;
//	BFile f;
//
//	if (find_directory(B_USER_SETTINGS_DIRECTORY, &p) == B_OK) {
//		p.Append("weather_parsed_data");
//
//		f.SetTo(p.Path(), B_WRITE_ONLY | B_ERASE_FILE | B_CREATE_FILE);
//		if (f.InitCheck() == B_OK)
//			parsedData.Flatten(&f);
//	}
// temp - save parsed data to file
	
	const int maxDaysForecast = 5;
	struct {
		BString	date;
		double 	maxTemperature;
		double 	minTemperature;
		double 	weatherCode;
	} dailyWeather[maxDaysForecast];

	BString detailMessage;
	if (!(parsedData.FindString("detail", &detailMessage) == B_OK)) {		
		//Get name of location
		BString title = "Milan";
		// TO-DO: get city name from Geocoding service;
		//parsedData.FindString("title", &title);
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
		
		int dayCount = 0;
		

		
		if (parsedData.FindMessage("daily", &weatherData) == B_OK) {
			
			for (int32 i = 0; weatherData.GetInfo(B_MESSAGE_TYPE, i, &name, &type, &count) == B_OK; i++) {
				
				// time unroll
				if (strcmp(name,"time")==0) {
					BMessage dayMessage;
					if (weatherData.FindMessage(name, &dayMessage) == B_OK) {
				
						BString date;
						char yearStr[4];
						char monthStr[2];
						char dayStr[2];
						int weekDay;
		
						char id[1] = "";
						sprintf(id,"%d",i);
						
						int tCount;
						uint32 tType;
						char *tName;
						for (int32 tDay = 0; dayMessage.GetInfo(B_STRING_TYPE, tDay, &tName, &tType, &tCount) == B_OK && tDay < maxDaysForecast; tDay++) {
							
							if (dayMessage.FindString(tName,&date) == B_OK)
								dailyWeather[tDay].date = date.String();
						}
					}
				}
				
				// min temperature unroll
				if (strcmp(name,"temperature_2m_min")==0) {
					BMessage tempMessage;
					if (weatherData.FindMessage(name, &tempMessage) == B_OK) {
				
						double minTemperature;
						
						int tCount;
						uint32 tType;
						char *tName;
						for (int32 tDay = 0; tempMessage.GetInfo(B_DOUBLE_TYPE, tDay, &tName, &tType, &tCount) == B_OK && tDay < maxDaysForecast; tDay++) {
							
							if (tempMessage.FindDouble(tName,&minTemperature) == B_OK)
								dailyWeather[tDay].minTemperature = (int)minTemperature;
						}
					}
				}
				
				// max temperature unroll
				if (strcmp(name,"temperature_2m_max")==0) {
					BMessage tempMessage;
					if (weatherData.FindMessage(name, &tempMessage) == B_OK) {
				
						double minTemperature;
						
						int tCount;
						uint32 tType;
						char *tName;
						for (int32 tDay = 0; tempMessage.GetInfo(B_DOUBLE_TYPE, tDay, &tName, &tType, &tCount) == B_OK && tDay < maxDaysForecast; tDay++) {
							
							if (tempMessage.FindDouble(tName,&minTemperature) == B_OK)
								dailyWeather[tDay].maxTemperature = (int)minTemperature;
						}
					}
				}
				
				// weathercodeunroll
				if (strcmp(name,"weathercode")==0) {
					BMessage codeMessage;
					if (weatherData.FindMessage(name, &codeMessage) == B_OK) {
				
						double code;
						
						int tCount;
						uint32 tType;
						char *tName;
						for (int32 tDay = 0; codeMessage.GetInfo(B_DOUBLE_TYPE, tDay, &tName, &tType, &tCount) == B_OK && tDay < maxDaysForecast; tDay++) {
							
							if (codeMessage.FindDouble(tName,&code) == B_OK)
								dailyWeather[tDay].weatherCode = (int)code;
						}
					}
				}
				
			}
		}
	}			
				
			/*
			
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
		}*/
		
		for (int tDay = 0; tDay < maxDaysForecast; tDay++) {
			BMessage *message = new BMessage(kForecastDataMessage);
			message->AddInt32("forecast", tDay);
			message->AddInt32("high", (int) dailyWeather[tDay].maxTemperature);
			message->AddInt32("low", (int) dailyWeather[tDay].minTemperature);
			message->AddInt32("condition", dailyWeather[tDay].weatherCode);
			
			
			message->AddString("day", "Tue");
			messenger.SendMessage(message);
			delete message;
		}
		
		//Get current weather
		BMessage* message = new BMessage(kDataMessage);
		message->AddInt32("temp", (int) 40);
		message->AddInt32("condition", 1);
		message->AddString("text", "test");
		messenger.SendMessage(message);
		delete message;
		

}

void
WSOpenMeteo::_ProcessCityData(bool success)
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
