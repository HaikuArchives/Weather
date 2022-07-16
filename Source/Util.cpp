/*
 * Copyright 2018 Benjamin Amos
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>

#include "ForecastView.h"
#include "Util.h"


status_t
LoadSettings(BMessage& m)
{
	BPath p;
	BFile f;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &p) != B_OK)
		return B_ERROR;
	p.Append(kSettingsFileName);

	f.SetTo(p.Path(), B_READ_ONLY);
	if (f.InitCheck() != B_OK)
		return B_ERROR;

	if (m.Unflatten(&f) != B_OK)
		return B_ERROR;

	if (m.what != kSettingsMessage)
		return B_ERROR;

	return B_OK;
}
