/*
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include <Json.h>
#include <Messenger.h>
#include <stdio.h>

#include "NetListener.h"


NetListener::NetListener(BHandler* handler, RequestType requestType)
	:
	BUrlProtocolListener(),
	fHandler(handler),
	fRequestType(requestType)
{
}


NetListener::~NetListener()
{
}


void
NetListener::ResponseStarted(BUrlRequest* caller)
{
}


void
NetListener::DataReceived(
	BUrlRequest* caller, const char* data, off_t position, ssize_t size)
{
	fResponseData.Write(data, size);
}


void
NetListener::RequestCompleted(BUrlRequest* caller, bool success)
{
	if (fRequestType == WEATHER_REQUEST)
		_ProcessWeatherData(success);

	if (fRequestType == CITY_REQUEST)
		_ProcessCityData(success);
}


void
NetListener::_ProcessWeatherData(bool success)
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

	BMessage conditionMessage;
	BMessage forecastMessage;
	BMessage queryMessage, resultsMessage, channelMessage, locationMessage;
	if (parsedData.FindMessage("query", &queryMessage) == B_OK
		&& queryMessage.FindMessage("results", &resultsMessage) == B_OK
		&& resultsMessage.FindMessage("channel", &channelMessage) == B_OK) {
		// Location: Retrieve Full City Name
		if (channelMessage.FindMessage("location", &locationMessage) == B_OK) {
			BString city, country, region;

			locationMessage.FindString("city", &city);
			locationMessage.FindString("country", &country);
			locationMessage.FindString("region", &region);

			BMessage* message = new BMessage(kUpdateCityName);
			message->AddString("city", city);
			messenger.SendMessage(message);
		}

		// TTL: Retrieve Time To live
		BString timeToLive;
		if (channelMessage.FindString("ttl", &timeToLive) == B_OK) {
			int ttl;
			sscanf(timeToLive.String(), "%d", &ttl);
			BMessage* message = new BMessage(kUpdateTTLMessage);
			message->AddInt32("ttl", ttl);
			messenger.SendMessage(message);
		}

		BMessage itemMessage;
		if (channelMessage.FindMessage("item", &itemMessage) == B_OK) {
			// Current Condition
			if (itemMessage.FindMessage("condition", &conditionMessage)
				== B_OK) {
				BString code;
				BString temp;
				BString text;

				conditionMessage.FindString("code", &code);
				conditionMessage.FindString("temp", &temp);
				conditionMessage.FindString("text", &text);

				// convert to integers
				int temperature, condition;
				sscanf(temp.String(), "%d", &temperature);
				sscanf(code.String(), "%d", &condition);

				BMessage* message = new BMessage(kDataMessage);
				message->AddInt32("temp", temperature);
				message->AddInt32("code", condition);
				message->AddString("text", text);
				messenger.SendMessage(message);
			}
			// Forecast
			if (itemMessage.FindMessage("forecast", &forecastMessage) == B_OK) {
				char* name;
				uint32 type;
				int32 count;
				BString code, highStr, lowStr, text, day;

				for (int32 i = 0; forecastMessage.GetInfo(
									  B_MESSAGE_TYPE, i, &name, &type, &count)
					 == B_OK;
					i++) {
					BMessage forecastDayMessage;
					if (forecastMessage.FindMessage(name, &forecastDayMessage)
						== B_OK) {
						forecastDayMessage.FindString("code", &code);
						forecastDayMessage.FindString("low", &lowStr);
						forecastDayMessage.FindString("high", &highStr);
						forecastDayMessage.FindString("text", &text);
						forecastDayMessage.FindString("day", &day);

						// convert to integers
						int high, low, condition;
						sscanf(highStr.String(), "%d", &high);
						sscanf(lowStr.String(), "%d", &low);
						sscanf(code.String(), "%d", &condition);

						BMessage* message = new BMessage(kForecastDataMessage);
						message->AddInt32("forecast", i);
						message->AddInt32("high", high);
						message->AddInt32("low", low);
						message->AddInt32("code", condition);
						message->AddString("text", text);
						message->AddString("day", day);
						messenger.SendMessage(message);
					}
				}
			}
		}
	}
}


void
NetListener::_ProcessCityData(bool success)
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

	BMessage queryMessage, resultsMessage, placeMessage;
	BString woeid;

	if (parsedData.FindMessage("query", &queryMessage) == B_OK
		&& queryMessage.FindMessage("results", &resultsMessage) == B_OK
		&& resultsMessage.FindMessage("place", &placeMessage) == B_OK
		&& placeMessage.FindString("woeid", &woeid) == B_OK) {
		BMessage* message = new BMessage(kDataMessage);
		message->AddString("id", woeid);
		messenger.SendMessage(message);
	}
	if (parsedData.FindMessage("query", &queryMessage) == B_OK
		&& queryMessage.FindMessage("results", &resultsMessage) == B_OK
		&& resultsMessage.FindMessage("place", &placeMessage) == B_OK) {
		int32 index = 0;
		BMessage placeIndexMessage;
		BString messageIndex("0");
		BMessage* message = new BMessage(kCitiesListMessage);
		while (placeMessage.FindMessage(messageIndex, &placeIndexMessage)
			== B_OK) {
			BMessage countryMessage, admin1Message, localityMessage;
			BString country, admin1, locality, woeid, countryCode;
			if (placeIndexMessage.FindMessage("country", &countryMessage)
				== B_OK) {
				countryMessage.FindString("content", &country);
				countryMessage.FindString("code", &countryCode);
			}
			if (placeIndexMessage.FindMessage("admin1", &admin1Message) == B_OK)
				admin1Message.FindString("content", &admin1);

			if (placeIndexMessage.FindMessage("locality1", &localityMessage)
				== B_OK) {
				localityMessage.FindString("content", &locality);
				localityMessage.FindString("woeid", &woeid);
			}
			BString total;
			total << locality << ", " << admin1 << ", " << country;
			message->AddString("city", total);
			message->AddString("woeid", woeid);
			message->AddString("countryCode", countryCode);
			index++;
			messageIndex = "";
			messageIndex << index;
		}
		messenger.SendMessage(message);
	} else {
		BMessage* message = new BMessage(kFailureMessage);
		messenger.SendMessage(message);
	}
}
