# Customize below to fit your system

# paths
PREFIX = /usr/local

# libs
L=xcb xcb-aux

# flags
CFLAGS = -std=gnu99 -Wall -g $(CPPFLAGS) $(foreach lib,$(L),$(shell pkg-config --cflags $(lib)))
LDFLAGS = $(foreach lib,$(L),$(shell pkg-config --libs $(lib)))
