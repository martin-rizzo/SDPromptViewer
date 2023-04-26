
# Dependencies (GLIB,GTK,LIBPEAS-GTK,EOG)
GLIB_CFLAGS    := $(shell pkg-config --cflags glib-2.0)
GTK_CFLAGS     := $(shell pkg-config --cflags gtk+-3.0)
LIBPEAS_CFLAGS := $(shell pkg-config --cflags libpeas-gtk-1.0)
EOG_CFLAGS     := $(shell pkg-config --cflags eog)


CC = gcc
CFLAGS = -Wall -fPIC $(GLIB_CFLAGS) $(GTK_CFLAGS) $(LIBPEAS_CFLAGS) $(EOG_CFLAGS)

SRCS = sdprompt-viewer-plugin.c sdprompt-viewer-preferences.c sdprompt-viewer-resources.c
OBJS = $(SRCS:.c=.o)


# Define variables
PROJECT_NAME := sdprompt-viewer
GETTEXT_PACKAGE := $(shell echo $(PROJECT_NAME) | tr '[:lower:]' '[:upper:]')

RESOURCES_C := $(PROJECT_NAME)-resources.c
CONFIG_H    := config.h
GSCHEMA_INPUT := system/sdprompt-viewer.gschema.xml.in

PLUGIN  := $(PROJECT_NAME).plugin
LIBRARY := lib$(PROJECT_NAME).so
GSCHEMA := org.gnome.eog.plugins.sdprompt-viewer.gschema.xml


all: $(PLUGIN) $(LIBRARY) $(GSCHEMA) 

%.o: %.c $(CONFIG_H) $(RESOURCES_C)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(PLUGIN) $(LIBRARY) $(GSCHEMA) $(RESOURCES_C) $(CONFIG_H)


#install-gschema: org.gnome.eog.plugins.sdprompt-viewer.gschema.xml
#	@echo "Installing gschema file..."
#	@install -D -m 644 $< $(DESTDIR)$(GSETTINGS_SCHEMAS_DIR)/$<

#uninstall-gschema:
#	@echo "Uninstalling gschema file..."
#	@rm -f $(DESTDIR)$(GSETTINGS_SCHEMAS_DIR)/org.gnome.eog.plugins.sdprompt-viewer.gschema.xml


#-------------------------------------------------------------------
# Generate "*-resources.c" (intermediate file)
#
$(RESOURCES_C): resources.xml
	glib-compile-resources --target="$@" --generate-header \
		resources.xml


#-------------------------------------------------------------------
# Generate "config.h" (intermediate file)
#
$(CONFIG_H):
	printf "#define GETTEXT_PACKAGE \"$(GETTEXT_PACKAGE)\"\n" > $@

#-------------------------------------------------------------------
# Generate "*-gschema.xml"
#
$(GSCHEMA): $(GSCHEMA_INPUT)
	@echo "Generating gschema file..."
	@sed -e 's/@GETTEXT_PACKAGE@/$(GETTEXT_PACKAGE)/g' $< > $@

#-------------------------------------------------------------------
# Generate "lib*.so"
#
$(LIBRARY): $(OBJS)
	$(CC) -shared -o $@ $^

#-------------------------------------------------------------------
# Generate "*.plugin"
#

PO_DIR := po

# To generate a .pot (translation template) file from POTFILES.in:
# > xgettext --output=messages.pot --files-from=po/POTFILES.in
#
# To generate a .po (translation file) from the .pot template:
# > msginit --input=messages.pot --output-file=es.po --locale=es
#
# To update an existing .po translation file from the .pot template:
# > msgmerge --update es.po messages.pot

# msgfmt --desktop --template myapp.desktop.in -d po -o myapp.desktop

$(PLUGIN): system/sdprompt-viewer.plugin.desktop.in
	msgfmt --desktop --keyword=Name --keyword=Description --template $< -d $(PO_DIR) -o $@


#PLUGIN_DATA_DIR := plugin_data_dir
#		##	install -D -m 644 $@ $(PLUGIN_DATA_DIR)/$@

