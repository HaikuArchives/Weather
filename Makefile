# Copyright (C) 2014 George White.
# Licensed under the MIT license.
# Made for Haiku.

NAME= Weather
TYPE= APP
APP_VERSI=
APP_MIME_SIG=

# The source les.
SRCS= \
	App.cpp

# Includes.
LOCAL_INCLUDE_PATHS = \
	{project}

# Resource files.
RDEFS=
RSRCS=

# Libraries.
LIBS= be
LIBPATHS=


include $(BUILDHOME)/etc/makefile-engine
