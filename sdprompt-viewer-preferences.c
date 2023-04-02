/**
 * @file    sdprompt-viewer-preferences.c
 * @brief   This file defines the "Preferences" object for the plugin.
 * @date    Mar 26, 2023
 * @author  Martin Rizzo | <martinrizzo@gmail.com>
 * @license http://www.opensource.org/licenses/mit-license.html [MIT License]
 *//*-------------------------------------------------------------------------
                       Stable Diffusion Prompt Viewer
  An "Eye of GNOME" plugin to view prompts that are embedded within the images
             
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

static GtkWidget * get_widget(GtkBuilder *builder, const gchar *widget_name) {
    return GTK_WIDGET( gtk_builder_get_object( builder, widget_name ) );
}


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
    eog_debug_message( DEBUG_PLUGINS, "SDPromptViewerPreferences disposing");
    G_OBJECT_CLASS( sdprompt_viewer_preferences_parent_class )
        ->dispose(object);
}

/*---------------------------- USER INTERFACE -----------------------------*/

static GtkWidget *
create_user_interface( PeasGtkConfigurable *configurable )
{
    GSettings *settings;
    GtkBuilder *config_builder;
    GError *error = NULL;
    GtkWidget *force_visibility_button;
    GtkWidget *force_width_check_button;
    GtkWidget *force_width_spin_button;
    GObject *result;
    gchar *object_ids[] = {"vbox1", NULL};

    settings = g_settings_new( SDPROMPT_VIEWER_GSCHEMA_ID );

    config_builder = gtk_builder_new ();
    gtk_builder_set_translation_domain (config_builder, GETTEXT_PACKAGE);
    if (!gtk_builder_add_objects_from_resource (config_builder, PREFERENCES_UI, object_ids, &error))
    {
        g_warning ("Couldn't load UI resource: %s", error->message);
        g_error_free (error);
    }

    // Add a reference to keep the box alive after the builder is gone
    result = g_object_ref( gtk_builder_get_object(config_builder, "vbox1") );
    
    
    /*-- binding widgets to plugin settings --*/
    force_visibility_button  = get_widget( config_builder, "force_visibility_button" );
    force_width_check_button = get_widget( config_builder, "force_width_check_button" );
    force_width_spin_button  = get_widget( config_builder, "force_width_spin_button" );
    
    gtk_spin_button_configure( force_width_spin_button,
                               gtk_adjustment_new(480,100,1000,5,50,0), 1, 0);

    g_settings_bind( settings, SETTINGS_FORCE_VISIBILITY,
                     force_visibility_button, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind( settings, SETTINGS_FORCE_MINIMUM_WIDTH,
                     force_width_check_button, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind( settings, SETTINGS_MINIMUM_WIDTH,
                     force_width_spin_button, "value", G_SETTINGS_BIND_DEFAULT);

    g_object_unref (config_builder);
    g_object_unref (settings);

    return GTK_WIDGET(result);
}

static void
peas_gtk_configurable_iface_init( PeasGtkConfigurableInterface *iface )
{
    iface->create_configure_widget = create_user_interface;
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
