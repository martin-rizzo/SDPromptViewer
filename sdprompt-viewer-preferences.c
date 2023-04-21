/**
 * @file    sdprompt-viewer-preferences.c
 * @brief   Implementation of the "Preferences" user interface for the plugin.
 * @author  Martin Rizzo | <martinrizzo@gmail.com>
 * @date    Mar 26, 2023
 * @repo    https://github.com/martin-rizzo/SDPromptViewer
 * @license MIT
 *//*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                      Stable Diffusion Prompt Viewer
      A plugin for "Eye of GNOME" that displays SD embedded prompts.
  
     Copyright (c) 2023 Martin Rizzo
  
     Permission is hereby granted, free of charge, to any person obtaining
     a copy of this software and associated documentation files (the
     "Software"), to deal in the Software without restriction, including
     without limitation the rights to use, copy, modify, merge, publish,
     distribute, sublicense, and/or sell copies of the Software, and to
     permit persons to whom the Software is furnished to do so, subject to
     the following conditions:
  
     The above copyright notice and this permission notice shall be
     included in all copies or substantial portions of the Software.
  
     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
     EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
     MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
     IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
     CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
     TORT OR OTHERWISE, ARISING FROM,OUT OF OR IN CONNECTION WITH THE
     SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <eog/eog-debug.h>
#include <libpeas-gtk/peas-gtk-configurable.h>

#include "resources.h"
#include "sdprompt-viewer-preferences.h"

static void
peas_gtk_configurable_iface_init( PeasGtkConfigurableInterface *iface );
static void
sdprompt_viewer_preferences_dispose( GObject *object );

static void create_gui( SDPromptViewerPreferences *preferences );
static void destroy_gui( SDPromptViewerPreferences *preferences );


#define get_widget(builder, widget_name) \
    GTK_WIDGET( gtk_builder_get_object( builder, widget_name) )

#define get_widgetx(builder, widget_type, widget_name) \
    widget_type( gtk_builder_get_object( builder, widget_name) )

#define settings_bind_default( settings, key, builder, widget_name, property ) \
    g_settings_bind( settings, key, \
                     gtk_builder_get_object(builder,widget_name), property, \
                     G_SETTINGS_BIND_DEFAULT )


/*
  G_DEFINE_DYNAMIC_TYPE_EXTENDED defines or references the following items:
    TYPES:
      * {TypeName}      : Structure defining instance properties.
      * {TypeName}Class : Structure defining class properties.
    VARS:
      * {type_name}_parent_class  : a pointer to the parent class.
    FUNCTIONS:
      * {type_name}_class_init    : Registers signals and properties specific to the class.
      * {type_name}_class_finalize: Frees resources allocated to the class.
      * {type_name}_init          : Initializes class-specific data fields.
      * {type_name}_get_type      : Returns the a GType that identifiers the class
*/
G_DEFINE_DYNAMIC_TYPE_EXTENDED(
    SDPromptViewerPreferences,      /* TypeName    */
    sdprompt_viewer_preferences,    /* type_name   */
    PEAS_TYPE_EXTENSION_BASE,       /* TYPE_PARENT */
    0,                              /* flags       */
    G_IMPLEMENT_INTERFACE_DYNAMIC(
        PEAS_GTK_TYPE_CONFIGURABLE,
        peas_gtk_configurable_iface_init
    )
)

/*----------------------  INITIALIZATION/DESTRUCTION ----------------------*/

static void
sdprompt_viewer_preferences_class_init( SDPromptViewerPreferencesClass *klass )
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = sdprompt_viewer_preferences_dispose;
}

static void
sdprompt_viewer_preferences_class_finalize( SDPromptViewerPreferencesClass *klass )
{
    /* needed for G_DEFINE_DYNAMIC_TYPE_EXTENDED */
}

static void
sdprompt_viewer_preferences_init( SDPromptViewerPreferences *object )
{
    /* needed for G_DEFINE_DYNAMIC_TYPE_EXTENDED */
}

static void
sdprompt_viewer_preferences_dispose( GObject *object )
{
    SDPromptViewerPreferences *preferences = SDPROMPT_VIEWER_PREFERENCES( object );

    eog_debug_message( DEBUG_PLUGINS, "SDPromptViewerPreferences disposing");
    destroy_gui( preferences );
    
    G_OBJECT_CLASS( sdprompt_viewer_preferences_parent_class )
        ->dispose(object);
}

/*----------------------- GRAPHICAL USER INTERFACE ------------------------*/

static void create_gui( SDPromptViewerPreferences *preferences )
{
    GtkBuilder *builder; GSettings *settings; GError *error = NULL;
    gchar *objects_to_build[] =
    {
        "main_container",
        "force_width_adjust",
        "border_size_adjust",
        "font_size_adjust",
        NULL
    };
    
    /* create builder */
    builder = gtk_builder_new();
    gtk_builder_set_translation_domain( builder,
                                        GETTEXT_PACKAGE );
    gtk_builder_add_objects_from_resource( builder,
                                           RES_PREFERENCES_UI,
                                           objects_to_build,
                                           &error );
    if( error ) {
        g_warning( "Couldn't load UI resource: %s", error->message );
        g_error_free (error);
    }
    
    /* bind widgets with plugin settings */
    settings = g_settings_new( SDPROMPT_VIEWER_GSCHEMA_ID );
    
    settings_bind_default( settings, SETTINGS_FORCE_VISIBILITY,
                           builder, "force_visibility_button", "active" );
    
    settings_bind_default( settings, SETTINGS_FORCE_MINIMUM_WIDTH,
                           builder, "force_width_check_button", "active" );
    
    settings_bind_default( settings, SETTINGS_MINIMUM_WIDTH,
                           builder, "force_width_spin_button", "value" );
    
    settings_bind_default( settings, SETTINGS_VISUAL_STYLE,
                           builder, "visual_style_combo_box", "active" );
    
    settings_bind_default( settings, SETTINGS_BORDER_SIZE,
                           builder, "border_size_adjust", "value" );
    
    settings_bind_default( settings, SETTINGS_FONT_SIZE,
                           builder, "font_size_adjust", "value" );
    
    g_object_unref( settings );
    settings = NULL;

    /* store builder and return */
    preferences->builder = builder;
}

static void destroy_gui( SDPromptViewerPreferences *preferences )
{
    if( preferences->builder ) {
        g_object_unref( preferences->builder );
        preferences->builder = NULL;
    }
}


static GtkWidget *
create_configure_widget( PeasGtkConfigurable *configurable )
{
    SDPromptViewerPreferences *preferences = SDPROMPT_VIEWER_PREFERENCES( configurable );
    create_gui( preferences );
    return g_object_ref(
        get_widget( preferences->builder, "main_container" )
    );
}

static void
peas_gtk_configurable_iface_init( PeasGtkConfigurableInterface *iface )
{
    iface->create_configure_widget = create_configure_widget;
}

/*========================= PLUGIN MAIN FUNCTION ==========================*/

void
sdprompt_viewer_preferences_register_types( PeasObjectModule *module )
{
    sdprompt_viewer_preferences_register_type( G_TYPE_MODULE( module ) );
    peas_object_module_register_extension_type(
        module,
        PEAS_GTK_TYPE_CONFIGURABLE,       /* <- the interface to implement */
        TYPE_SDPROMPT_VIEWER_PREFERENCES  /* <- my object                  */
    );
}
