# Copyright (C) 2013 DNAnexus, Inc.
#
# This file is part of dx-toolkit (DNAnexus platform client libraries).
#
#   Licensed under the Apache License, Version 2.0 (the "License"); you may not
#   use this file except in compliance with the License. You may obtain a copy
#   of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#   WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#   License for the specific language governing permissions and limitations
#   under the License.

ifndef DESTDIR
	export DESTDIR=/opt/dnanexus
endif
ifndef PREFIX
	export PREFIX=/
endif
UNAME := $(shell uname)

all:
	mkdir -pv build
ifeq ($(UNAME), MINGW32_NT-6.1)
	make -C build -f ../Makefile.mw all
else
	make -C build -f ../Makefile.static all
endif
clean:
	rm -rvf build

dist: all
ifeq ($(UNAME), MINGW32_NT-6.1)
	make -C build -f ../Makefile.mw dist
else
	make -C build -f ../Makefile.static dist
endif

install: all
	install -v build/ua $(DESTDIR)/$(PREFIX)/bin

.PHONY: all clean dist install
