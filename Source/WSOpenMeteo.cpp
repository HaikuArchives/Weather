/*
 * Copyright 2022 Davide Alfano (Nexus6) <nexus6.haiku@icloud.com>
 * Copyright 2020 Raheem Idowu <abdurraheemidowu@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 * Weather data by Open-Meteo.com at https://open-meteo.com/
 * under Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
 */

#include <Alert.h>
#include <Json.h>
#include <Messenger.h>
#include <memory>
#include <StorageKit.h>

#include <parsedate.h>
#include <stdio.h>
#include <string>

#include "MainWindow.h"
#include "PreferencesWindow.h"
#include "WSOpenMeteo.h"


WSOpenMeteo::WSOpenMeteo(BHandler* handler, BMallocIO* responseData, RequestType requestType)
	:
	BUrlProtocolListener(),
	fHandler(handler),
	fRequestType(requestType),
	fResponseData(responseData)
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
WSOpenMeteo::DataReceived(BUrlRequest* caller, const char* data, off_t position, ssize_t size)
{
}


void
WSOpenMeteo::RequestCompleted(BUrlRequest* caller, bool success)
{
	if (fRequestType == WEATHER_REQUEST)
		_ProcessWeatherData(success);

	if (fRequestType == CITY_REQUEST)
		_ProcessCityData(success);
}


BString
WSOpenMeteo::GetUrl(double longitude, double latitude, DisplayUnit unit)
{

	char lati[12];
	char longi[12];
	snprintf(lati, sizeof(lati), "%f", latitude);
	snprintf(longi, sizeof(longi), "%f", longitude);

	BString urlString("https://api.open-meteo.com/v1/forecast?latitude=");
	urlString
		<< lati << "&longitude=" << longi
		<< "&daily=weathercode,temperature_2m_max,temperature_2m_min&current_"
		   "weather=true&timeformat=unixtime&timezone=auto";

	// Temperature unit measure
	switch (unit) {
		case FAHRENHEIT:
			urlString << "&temperature_unit=fahrenheit";
			break;
		case CELSIUS:
			urlString << "&temperature_unit=celsius";
			break;
		default: // CELSIUS
			urlString << "&temperature_unit=celsius";
	}

	return urlString;
}


void
WSOpenMeteo::_ProcessWeatherData(bool success)
{
	BMessenger messenger(fHandler);
	BString jsonString;

	if (!success) {
		BMessage* message = new BMessage(kFailureMessage);
		messenger.SendMessage(message);
		return;
	}

	jsonString.SetTo(static_cast<const char*>(fResponseData->Buffer()),
		fResponseData->BufferLength());


	BMessage parsedData;
	BJson parser;

	status_t status = parser.Parse(jsonString, parsedData);
	if (status == B_BAD_DATA) {
		printf("JSON Parser error for data:\n%s\n", jsonString.String());
		BMessage* message = new BMessage(kFailureMessage);
		messenger.SendMessage(message);
		return;
	}

#if DEBUG
	SerializeBMessage(&parsedData, "weather_parsed_data");
#endif

	const int maxDaysForecast = 5;
	struct {
		double date;
		double maxTemperature;
		double minTemperature;
		double weatherCode;
	} dailyWeather[maxDaysForecast];

	BMessage* message = new BMessage(kUpdateCityName);
	messenger.SendMessage(message);
	
	// Get UTC timezone offset, use 0 as default
	double utc_offset;
	if (parsedData.FindDouble("utc_offset_seconds", &utc_offset) != B_OK)
		utc_offset = 0;

	// Get weather forecast
	char* name;
	uint32 type;
	int32 count, condition;
	double high, low, temp;
	BString code, text;		
		
	int dayCount = 0;

	BMessage weatherData;

	if (parsedData.FindMessage("daily", &weatherData) == B_OK) {

		for (int32 i = 0;
			weatherData.GetInfo(B_MESSAGE_TYPE, i, &name, &type, &count) == B_OK;
			i++) {

			// time unroll
			if (strcmp(name, "time") == 0) {
				BMessage dayMessage;
				if (weatherData.FindMessage(name, &dayMessage) == B_OK) {

					double date;
					int32 tCount;
					uint32 tType;
					char* tName;
					for (int32 tDay = 0; dayMessage.GetInfo(B_DOUBLE_TYPE, tDay,
											 &tName, &tType, &tCount) == B_OK
							 && tDay < maxDaysForecast;
							tDay++) {

						if (dayMessage.FindDouble(tName, &date) == B_OK)
							// Offset is added to get the correct date for the timezone selected
							dailyWeather[tDay].date = date + utc_offset; 
					}
				}
			}

			// min temperature unroll
			if (strcmp(name, "temperature_2m_min") == 0) {
				BMessage tempMessage;
				if (weatherData.FindMessage(name, &tempMessage) == B_OK) {

					double minTemperature;

					int32 tCount;
					uint32 tType;
					char* tName;
					for (int32 tDay = 0; tempMessage.GetInfo(B_DOUBLE_TYPE,
											 tDay, &tName, &tType, &tCount) == B_OK
							&& tDay < maxDaysForecast;
							tDay++) {

						if (tempMessage.FindDouble(tName, &minTemperature) == B_OK)
							dailyWeather[tDay].minTemperature = (int) minTemperature;
					}
				}
			}

			// max temperature unroll
			if (strcmp(name, "temperature_2m_max") == 0) {
				BMessage tempMessage;
				if (weatherData.FindMessage(name, &tempMessage) == B_OK) {

					double minTemperature;

					int32 tCount;
					uint32 tType;
					char* tName;
					for (int32 tDay = 0; tempMessage.GetInfo(B_DOUBLE_TYPE,
											 tDay, &tName, &tType, &tCount) == B_OK
							 && tDay < maxDaysForecast;
							tDay++) {

						if (tempMessage.FindDouble(tName, &minTemperature) == B_OK)
							dailyWeather[tDay].maxTemperature = (int) minTemperature;
					}
				}
			}

			// weathercode unroll
			if (strcmp(name, "weathercode") == 0) {
				BMessage codeMessage;
				if (weatherData.FindMessage(name, &codeMessage) == B_OK) {

					double code;

					int32 tCount;
					uint32 tType;
					char* tName;
					for (int32 tDay = 0; codeMessage.GetInfo(B_DOUBLE_TYPE, tDay, &tName, &tType, 							&tCount) == B_OK && tDay < maxDaysForecast; tDay++) {

						if (codeMessage.FindDouble(tName, &code) == B_OK)
							dailyWeather[tDay].weatherCode = (int) code;
					}
				}
			}
		}
	}

	// Get forecast
	for (int tDay = 0; tDay < maxDaysForecast; tDay++) {
		BMessage* message = new BMessage(kForecastDataMessage);
		message->AddInt32("forecast", tDay);
		message->AddInt32("high", (int) dailyWeather[tDay].maxTemperature);
		message->AddInt32("low", (int) dailyWeather[tDay].minTemperature);
		message->AddInt32("condition", dailyWeather[tDay].weatherCode);

		// get unix timestamp
		BDate date = BDate((time_t) dailyWeather[tDay].date);
		BString dayOfWeek;
		switch (date.DayOfWeek()) {
			case 1:
				dayOfWeek = "Mon";
				break;
			case 2:
				dayOfWeek = "Tue";
				break;
			case 3:
				dayOfWeek = "Wed";
				break;
			case 4:
				dayOfWeek = "Thu";
				break;
			case 5:
				dayOfWeek = "Fri";
				break;
			case 6:
				dayOfWeek = "Sat";
				break;
			case 7:
				dayOfWeek = "Sun";
				break;
			default:
				dayOfWeek = "Undefined";
		}

		message->AddString("day", dayOfWeek.String());
		messenger.SendMessage(message);
	}


	// Get current weather
	BMessage currentWeatherData;
	if (parsedData.FindMessage("current_weather", &currentWeatherData)
		== B_OK) {

		double temperature;
		double condition;
		double time;

		BMessage* currentMessage = new BMessage(kDataMessage);

		if (currentWeatherData.FindDouble("temperature", &temperature) == B_OK)
			currentMessage->AddInt32("temp", (int) temperature);

		if (currentWeatherData.FindDouble("weathercode", &condition) == B_OK)
			currentMessage->AddInt32("condition", (int) condition);

		messenger.SendMessage(currentMessage);
	}
}


void
WSOpenMeteo::SerializeBMessage(BMessage* message, BString fileName)
{
	BPath p;
	BFile f;

	if (find_directory(B_USER_LOG_DIRECTORY, &p) == B_OK) {
		p.Append(fileName);

		f.SetTo(p.Path(), B_WRITE_ONLY | B_ERASE_FILE | B_CREATE_FILE);
		if (f.InitCheck() == B_OK)
			message->Flatten(&f);
	}
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
	jsonString.SetTo(static_cast<const char*>(fResponseData->Buffer()),
		fResponseData->BufferLength());

	status_t status = parser.Parse(jsonString, parsedData);

#if DEBUG
	SerializeBMessage(&parsedData, "weather_parsed_location");
#endif

	if (status == B_BAD_DATA) {
		printf("JSON Parser error for data:\n%s\n", jsonString.String());
		BMessage* message = new BMessage(kFailureMessage);
		messenger.SendMessage(message);
		return;
	}

	char* name;
	uint32 type;
	int32 count;

	BMessage* message = new BMessage(kCitiesListMessage);

	BMessage results;
	if (parsedData.FindMessage("results", &results) == B_OK) {
		for (int32 i = 0;
			results.GetInfo(B_MESSAGE_TYPE, i, &name, &type, &count) == B_OK;
			i++) {

			BMessage locationMessage;
			BString locationName = "";
			double latitude = 0L;
			double longitude = 0L;
			double countryId = 0L;
			double id = 0L;
			BString country = "";
			BString admin1 = "";
			BString admin2 = "";
			BString admin3 = "";
			BString extendedInfo = "";

			if (results.FindMessage(name, &locationMessage) == B_OK) {
				locationMessage.FindDouble("id", &id);
				locationMessage.FindString("name", &locationName);
				locationMessage.FindString("country", &country);
				locationMessage.FindDouble("country_id", &countryId);
				locationMessage.FindString("admin1", &admin1);
				locationMessage.FindString("admin2", &admin2);
				locationMessage.FindString("admin3", &admin3);
				
				extendedInfo << locationName;
				if (admin3 != "" && admin3 != locationName && admin3 != country)
					extendedInfo << ", " << admin3;
				if (admin2 != "" && admin2 != locationName && admin2 != country)
					extendedInfo << ", "<< admin2;
				if (admin1 != "" && admin1 != locationName && admin1 != country)
					extendedInfo << ", "<< admin1;
				extendedInfo << ", " << country;
							 
				locationMessage.FindDouble("longitude", &longitude);
				locationMessage.FindDouble("latitude", &latitude);

				message->AddInt32("id", (int) id);
				message->AddString("city", locationName);
				message->AddString("country", country);
				message->AddInt32("country_id", countryId);
				message->AddString("extended_info", extendedInfo);
				message->AddDouble("longitude", longitude);
				message->AddDouble("latitude", latitude);
			}
		}
	}
	
#if DEBUG
	SerializeBMessage(message, "weather_location_message");
#endif
	messenger.SendMessage(message);
}
