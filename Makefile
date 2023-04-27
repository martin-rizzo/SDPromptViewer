# File    : Makefile
# Brief   : Compiles, installs, and uninstalls the SD Prompt Viewer plugin
# Author  : Martin Rizzo | <martinrizzo@gmail.com>
# Date    : Apr 25, 2023
# Repo    : https://github.com/martin-rizzo/SDPromptViewer
# License : MIT
#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#                      Stable Diffusion Prompt Viewer
#      A plugin for "Eye of GNOME" that displays SD embedded prompts.
#   
#     Copyright (c) 2023 Martin Rizzo
#     
#     Permission is hereby granted, free of charge, to any person obtaining
#     a copy of this software and associated documentation files (the
#     "Software"), to deal in the Software without restriction, including
#     without limitation the rights to use, copy, modify, merge, publish,
#     distribute, sublicense, and/or sell copies of the Software, and to
#     permit persons to whom the Software is furnished to do so, subject to
#     the following conditions:
#     
#     The above copyright notice and this permission notice shall be
#     included in all copies or substantial portions of the Software.
#     
#     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#     EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#     MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#     IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#     CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#     TORT OR OTHERWISE, ARISING FROM,OUT OF OR IN CONNECTION WITH THE
#     SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _

CC = gcc
CFLAGS = -Wall -fPIC $(GLIB_CFLAGS) $(GTK_CFLAGS) $(LIBPEAS_CFLAGS) $(EOG_CFLAGS)

# Dependencies (GLIB,GTK,LIBPEAS-GTK,EOG)
GLIB_CFLAGS    := $(shell pkg-config --cflags glib-2.0)
GTK_CFLAGS     := $(shell pkg-config --cflags gtk+-3.0)
LIBPEAS_CFLAGS := $(shell pkg-config --cflags libpeas-gtk-1.0)
EOG_CFLAGS     := $(shell pkg-config --cflags eog)

# Directories
XDG_DATA_HOME ?= $(HOME)/.local/share
EOG_PLUGINS_DIR := $(XDG_DATA_HOME)/eog/plugins
GIO_SCHEMAS_DIR := $(XDG_DATA_HOME)/glib-2.0/schemas
PO_DIR          := po

# Define variables
PROJECT_NAME := sdprompt-viewer
GETTEXT_PACKAGE := $(shell echo $(PROJECT_NAME) | tr '[:lower:]' '[:upper:]')

GSCHEMA_INPUT := system/sdprompt-viewer.gschema.xml.in
PLUGIN_INPUT  := system/sdprompt-viewer.plugin.desktop.in
CONFIG_H      := config.h
RESOURCES_C   := $(PROJECT_NAME)-resources.c

LIBRARY := lib$(PROJECT_NAME).so
PLUGIN  := $(PROJECT_NAME).plugin
GSCHEMA := org.gnome.eog.plugins.sdprompt-viewer.gschema.xml

SRCS = sdprompt-viewer-plugin.c sdprompt-viewer-preferences.c $(RESOURCES_C)
OBJS = $(SRCS:.c=.o)



# List of targets
.PHONY: all clean install remove run info

# Target to build all the components
all: $(PLUGIN) $(LIBRARY) $(GSCHEMA) 

# Target to clean all the build artifacts
clean:
	rm -f $(OBJS) $(PLUGIN) $(LIBRARY) $(GSCHEMA) $(RESOURCES_C) $(CONFIG_H)

# Target to install the plugin
install: $(PLUGIN) $(LIBRARY) $(GSCHEMA)
	install -D $(PLUGIN)  $(EOG_PLUGINS_DIR)/$(PLUGIN)
	install -D $(LIBRARY) $(EOG_PLUGINS_DIR)/$(LIBRARY)
	install -D $(GSCHEMA) $(GIO_SCHEMAS_DIR)/$(GSCHEMA)
	glib-compile-schemas $(GIO_SCHEMAS_DIR)

# Target to uninstall the plugin
remove:
	rm -f "$(EOG_PLUGINS_DIR)/$(LIBRARY)"
	rm -f "$(EOG_PLUGINS_DIR)/$(PLUGIN)"
	rm -f "$(GIO_SCHEMAS_DIR)/$(GSCHEMA)"
	glib-compile-schemas "$(GIO_SCHEMAS_DIR)"

# Target to execute EOG
run:
	EOG_DEBUG_PLUGINS=true GOBJECT_DEBUG=instance-count eog

# Target to displays internal operational info of the Makefile
info:
	@echo "Makefile for building and installing the EOG plugin."
	@echo "  EOG_PLUGINS_DIR = $(EOG_PLUGINS_DIR)"
	@echo "  GIO_SCHEMAS_DIR = $(GIO_SCHEMAS_DIR)"


#-------------------------------------------------------------------
# Generate ".o" files from C source code
#
%.o: %.c $(CONFIG_H) $(RESOURCES_C)
	$(CC) $(CFLAGS) -c $< -o $@

#-------------------------------------------------------------------
# Generate "config.h" (intermediate file)
#
$(CONFIG_H):
	printf "#define GETTEXT_PACKAGE \"$(GETTEXT_PACKAGE)\"\n" > $@

#-------------------------------------------------------------------
# Generate "*-resources.c" (intermediate file)
#
$(RESOURCES_C): resources.xml
	glib-compile-resources --target="$@" --generate-source \
		resources.xml

#-------------------------------------------------------------------
# Generate "lib*.so"
#
$(LIBRARY): $(OBJS)
	$(CC) -shared -o $@ $^

#-------------------------------------------------------------------
# Generate "*-gschema.xml"
#
$(GSCHEMA): $(GSCHEMA_INPUT)
	@echo "Generating gschema file..."
	@sed -e 's/@GETTEXT_PACKAGE@/$(GETTEXT_PACKAGE)/g' $< > $@

#-------------------------------------------------------------------
# Generate "*.plugin"
#
$(PLUGIN): $(PLUGIN_INPUT)
	@echo "Generating plugin file..."
	@msgfmt --desktop --keyword=Name --keyword=Description \
	        --template $< -d $(PO_DIR) -o $@


