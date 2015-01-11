/*
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Messenger.h>

#include "NetListener.h"


NetListener::NetListener(BLooper* parent)
	:
	BUrlProtocolListener()
{
	fParent = parent;
}


void NetListener::ResponseStarted(BUrlRequest* caller) {
	fResponse = "";
}


void NetListener::DataReceived(BUrlRequest* caller, const char* data,
	off_t position, ssize_t size) {
	char* buf = new char[size + 1];
	memcpy(buf, data, size);
	buf[size] = '\0';
	fResponse << buf;
}

#include <stdio.h>
void NetListener::RequestCompleted(BUrlRequest* caller,
	bool success) {
	BMessenger* messenger = new BMessenger(fParent);
	
	// extract data
	BString temp = fResponse;
	int first = fResponse.FindFirst("woeid>");
	int length = fResponse.FindFirst("<", first + 6);
	
	if (first != -1) {
		temp.Truncate(length);
		temp.Remove(0, first + 6);
		
		BMessage* message = new BMessage(kDataMessage);
		message->AddString("id", temp);
		messenger->SendMessage(message);
		
		return;
	}
	
	first = fResponse.FindFirst("temp=\"");
	length = fResponse.FindFirst("\"", first + 6);
	
	if (first == -1) {
		BMessage* message = new BMessage(kFailureMessage);
		messenger->SendMessage(message);
		
		return;
	}
	
	temp.Truncate(length);
	temp.Remove(0, first + 6);
		
	BString code = fResponse;
	first = fResponse.FindFirst("code=\"");
	length = fResponse.FindFirst("\"", first + 6);
	code.Truncate(length);
	code.Remove(0, first + 6);
	
	BString text = fResponse;
	first = fResponse.FindFirst("text=\"");
	length = fResponse.FindLast("\"", first + 25);
	text.Truncate(length);
	text.Remove(0, first + 6);
	
	// convert to integers
	int temperature, condition;
	sscanf(temp.String(), "%d", &temperature);
	sscanf(code.String(), "%d", &condition);
	
	BMessage* message = new BMessage(kDataMessage);
	message->AddInt32("temp", temperature);
	message->AddInt32("code", condition);
	message->AddString("text", text);
	messenger->SendMessage(message);
	
	delete messenger;
}
