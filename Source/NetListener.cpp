/*
 * Copyright 2015 Przemysław Buczkowski <przemub@przemub.pl>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Messenger.h>

#include <stdio.h>

#include "MainWindow.h"
#include "NetListener.h"


NetListener::NetListener(MainWindow* parent)
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


void NetListener::RequestCompleted(BUrlRequest* caller,
	bool success) {
	// extract data
	BString temp = fResponse;
	int first = fResponse.FindFirst("temp=\"");
	int length = fResponse.FindLast("\"", first + 9);
	temp.Truncate(length);
	temp.Remove(0, first + 6);
		
	BString code = fResponse;
	first = fResponse.FindFirst("code=\"");
	length = fResponse.FindLast("\"", first + 9);
	code.Truncate(length);
	code.Remove(0, first + 6);
	
	BString text = fResponse;
	first = fResponse.FindFirst("text=\"");
	length = fResponse.FindLast("\"", first + 12);
	text.Truncate(length);
	text.Remove(0, first + 6);
	
	// convert to integers
	int temperature, condition;
	sscanf(temp.String(), "%d", &temperature);
	sscanf(code.String(), "%d", &condition);
	
	BMessenger* messenger = new BMessenger(fParent);
	BMessage* message = new BMessage(kDataMessage);
	message->AddInt32("temp", temperature);
	message->AddInt32("code", condition);
	message->AddString("text", text);
	messenger->SendMessage(message);
}
