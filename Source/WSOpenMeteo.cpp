/*
 * Copyright 2022 Davide Alfano (Nexus6) <nexus6.haiku@icloud.com>
 * Copyright 2020 Raheem Idowu <abdurraheemidowu@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 * Weather data source using the MetaWeather API at: metaweather.com
 */
#include <Json.h>
#include <Messenger.h>
#include <memory>
#include <parsedate.h>
#include <stdio.h>
#include <string>

// remove
#include <Alert.h>
#include <StorageKit.h>

#include "MainWindow.h"
#include "PreferencesWindow.h"
#include "WSOpenMeteo.h"


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
WSOpenMeteo::DataReceived(
	BUrlRequest* caller, const char* data, off_t position, ssize_t size)
{

	fResponseData.Write(data, size);
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
	// BString urlString("https://www.openmeteo.com/api/location/");
	// urlString << fCityId << "/";

	char lati[12];
	char longi[12];
	snprintf(lati, sizeof(lati), "%f", latitude);
	snprintf(longi, sizeof(longi), "%f", longitude);

	BString urlString("https://api.open-meteo.com/v1/forecast?latitude=");
	urlString
		<< lati << "&longitude=" << longi
		<< "&daily=weathercode,temperature_2m_max,temperature_2m_min&current_"
		   "weather=true&timeformat=unixtime&timezone=Europe%2FBerlin";

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

	//	double latitude = 45.49624;
	//	double longitude= 9.29323;
	//	BString
	//	urlString("https://api.open-meteo.com/v1/forecast?latitude=52.52&longitude=13.41&daily=weathercode,temperature_2m_max,temperature_2m_min&current_weather=true&timeformat=unixtime&timezone=Europe%2FBerlin");
	//	https://api.open-meteo.com/v1/forecast?latitude=52.52&longitude=13.41&daily=weathercode,temperature_2m_max,temperature_2m_min,sunrise,sunset&current_weather=true&timeformat=unixtime&timezone=Europe%2FBerlin
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


	SerializeBMessage(&parsedData, "weather_parsed_data");

	const int maxDaysForecast = 5;
	struct {
		double date;
		double maxTemperature;
		double minTemperature;
		double weatherCode;
	} dailyWeather[maxDaysForecast];

	//	BString detailMessage;
	//	if (!(parsedData.FindString("detail", &detailMessage) == B_OK)) {
	// Get name of location
	// BString title = "Milan";
	// TO-DO: get city name from Geocoding service;
	// parsedData.FindString("title", &title);
	BMessage* message = new BMessage(kUpdateCityName);
	// message->AddString("city", title);
	messenger.SendMessage(message);
	//	}

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
							dailyWeather[tDay].date = date; // date.String();
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
					for (int32 tDay = 0; codeMessage.GetInfo(B_DOUBLE_TYPE,
											 tDay, &tName, &tType, &tCount) == B_OK
							 && tDay < maxDaysForecast;
							tDay++) {

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

		// if (currentWeatherData.FindDouble("time",&time) == B_OK)
		//	currentMessage->AddString("text",
		//BString(std::to_string(temperature)));

		messenger.SendMessage(currentMessage);
	}
}


void
WSOpenMeteo::SerializeBMessage(BMessage* message, BString fileName)
{
	BPath p;
	BFile f;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &p) == B_OK) {
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
	jsonString.SetTo(static_cast<const char*>(fResponseData.Buffer()),
		fResponseData.BufferLength());

	status_t status = parser.Parse(jsonString, parsedData);

	SerializeBMessage(&parsedData, "weather_parsed_location");

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
				extendedInfo << locationName << ", " << country << " ("
							 << admin1 << ", " << admin2 << ", " << admin3
							 << ")";
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
	SerializeBMessage(message, "weather_location_message");
	messenger.SendMessage(message);
}
