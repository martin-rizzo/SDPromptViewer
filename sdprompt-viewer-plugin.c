/**
 * @file    sdprompt-viewer-plugin.c
 * @date    Mar 25, 2023
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
#include <eog/eog-image.h>
#include <eog/eog-thumb-view.h>
#include <eog/eog-sidebar.h>
#include <eog/eog-window-activatable.h>

#include "resources.h"
#include "utils_png.h"
#include "utils_jpgtx.h"
#include "utils_parser.h"
#include "utils_widget.h"
#include "sdprompt-viewer-plugin.h"
#include "sdprompt-viewer-preferences.h"

static void
eog_window_activatable_iface_init (EogWindowActivatableInterface *iface);
static void
sdprompt_viewer_plugin_dispose( GObject *object );
static void
sdprompt_viewer_plugin_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void
sdprompt_viewer_plugin_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);

enum {
    PROP_O,
    PROP_WINDOW,
    PROP_FORCE_MINIMUM_WIDTH,
    PROP_MINIMUM_WIDTH
};

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
    SDPromptViewerPlugin,      /* TypeName    */
    sdprompt_viewer_plugin,    /* type_name   */
    PEAS_TYPE_EXTENSION_BASE,  /* TYPE_PARENT */
    0,                         /* flags       */
    /* custom code that gets inserted in the *_register_type() function */
    G_IMPLEMENT_INTERFACE_DYNAMIC(
        EOG_TYPE_WINDOW_ACTIVATABLE, eog_window_activatable_iface_init)
)


/*----------------------  INITIALIZATION/DESTRUCTION ----------------------*/

static void
sdprompt_viewer_plugin_class_init( SDPromptViewerPluginClass *klass )
{    
    GObjectClass *object_class = G_OBJECT_CLASS( klass );
    object_class->dispose      = sdprompt_viewer_plugin_dispose;

    /* register properties */
    object_class->set_property = sdprompt_viewer_plugin_set_property;
    object_class->get_property = sdprompt_viewer_plugin_get_property;
    g_object_class_override_property( object_class, PROP_WINDOW, "window" );
    g_object_class_install_property( object_class, PROP_FORCE_MINIMUM_WIDTH,
        g_param_spec_boolean(
            "force-minimum-width", NULL, NULL, FALSE,
            G_PARAM_READWRITE | G_PARAM_STATIC_NAME));
    /*
    g_object_class_install_property(object_class, PROP_ENABLE_STATUSBAR,
        g_param_spec_boolean(
            "enable-statusbar", NULL, NULL, FALSE,
            G_PARAM_READWRITE | G_PARAM_STATIC_NAME));
    */
}

static void
sdprompt_viewer_plugin_class_finalize( SDPromptViewerPluginClass *klass )
{
    /* needed for G_DEFINE_DYNAMIC_TYPE_EXTENDED */
}

static void
sdprompt_viewer_plugin_init( SDPromptViewerPlugin *object )
{
    /* needed for G_DEFINE_DYNAMIC_TYPE_EXTENDED */
}

static void
sdprompt_viewer_plugin_dispose( GObject *object )
{
    SDPromptViewerPlugin *plugin = SDPROMPT_VIEWER_PLUGIN( object );
    eog_debug_message( DEBUG_PLUGINS, "SDPromptViewerPlugin" );
    
    g_clear_object( &plugin->window );

    G_OBJECT_CLASS( sdprompt_viewer_plugin_parent_class )->dispose( object );
}

/*------------------------------ PROPERTIES -------------------------------*/

static void
sdprompt_viewer_plugin_get_property(GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
    SDPromptViewerPlugin *plugin = SDPROMPT_VIEWER_PLUGIN( object );
    switch (prop_id)
    {
        case PROP_FORCE_MINIMUM_WIDTH:
            g_value_set_boolean(value, plugin->force_minimum_width);
            break;
        case PROP_WINDOW:
            g_value_set_object(value, plugin->window);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
sdprompt_viewer_plugin_set_property(GObject       *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
    SDPromptViewerPlugin *plugin = SDPROMPT_VIEWER_PLUGIN (object);
    switch (prop_id)
    {
        case PROP_FORCE_MINIMUM_WIDTH:
            plugin->force_minimum_width = g_value_get_boolean(value);
            break;
        case PROP_WINDOW:
            plugin->window = EOG_WINDOW( g_value_dup_object(value) );
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}


/*---------------------------- USER INTERFACE -----------------------------*/

static void
hide_all_widgets( SDPromptViewerPlugin *plugin )
{
    static const gchar *widgets_to_hide[] = {
        "message_label",
        "loading_spinner",
        NULL
    };
    int i;
    GtkBuilder *builder        = plugin->sidebar_builder;
    GtkWidget  *main_container = get_widget( builder, "main_container" );
    GtkWidget  *widget;
    
    hide_group_descendants( main_container );
    for( i=0 ; widgets_to_hide[i] ; ++i ) {
        widget = get_widget(builder, widgets_to_hide[i]);
        if( widget ) { gtk_widget_set_visible( widget, FALSE ); }
    }
    /* remove text from all widgets (GtkEntry & GtkTextView) */
    widget = get_widget(builder, "unknown_text_view");
    if( widget ) { clear_widget_text(widget); }
}

static void
show_spinner( SDPromptViewerPlugin *plugin )
{
    GtkBuilder *builder = plugin->sidebar_builder;
    GtkWidget *spinner = get_widget(builder, "loading_spinner");
    hide_all_widgets( plugin );
    gtk_widget_set_visible( spinner, TRUE );
}

static void
show_message( SDPromptViewerPlugin *plugin, const gchar *message )
{
    GtkBuilder *builder = plugin->sidebar_builder;
    GtkWidget *label = get_widget(builder, "message_label");
    hide_all_widgets( plugin );
    gtk_widget_set_visible( label, TRUE );
    gtk_label_set_label( GTK_LABEL(label), message);
}

static void
show_size( SDPromptViewerPlugin *plugin, const gchar *text, int text_size )
{
    const gchar *ptr; int size;
    GtkWidget *width_widget, *height_widget;
    GtkBuilder *builder = plugin->sidebar_builder;
    if( text_size<0 ) { text_size = strlen(text); }
    

    width_widget  = get_widget( builder, "width_entry" );
    height_widget = get_widget( builder, "height_entry" );
    if( width_widget && height_widget ) {
        ptr = text; size = text_size;
        while( size>0 && '0'<=*ptr && *ptr<='9' ) { --size; ++ptr; }
        set_widget_text( width_widget, text, (ptr-text) );
        if( size>0) { --size; ++ptr; }
        set_widget_text( height_widget, ptr, size );
        show_group_ancestor( height_widget );
    }
}

static void
show_unknown( SDPromptViewerPlugin *plugin,
             const gchar *key_name,
             const gchar *text, int text_size)
{
    GtkWidget *group_widget;
    GtkTextView *text_view; GtkTextBuffer *buffer;
    GtkBuilder *builder = plugin->sidebar_builder;
    if( text_size<0 ) { text_size = strlen(text); }

    text_view = GTK_TEXT_VIEW( get_widget( builder, "unknown_text_view" ) );
    buffer    = text_view ? gtk_text_view_get_buffer( text_view ) : NULL;
    if( buffer ) {
        gtk_text_buffer_insert_at_cursor( buffer, key_name, -1 );
        gtk_text_buffer_insert_at_cursor( buffer, ": "    , -1 );
        gtk_text_buffer_insert_at_cursor( buffer, text , text_size );
        gtk_text_buffer_insert_at_cursor( buffer, "\n"    , -1 );
        group_widget = get_widget( builder, "unknown_group" );
        if( group_widget ) { gtk_widget_set_visible( group_widget, TRUE ); }
    }
}


static void
show_sd_parameter( SDPromptViewerPlugin *plugin,
                   const gchar *key,
                   const gchar *value)
{
#   define IF_EQUAL(key,key_name) if(0==strcmp(key,key_name))
    GtkBuilder *builder = plugin->sidebar_builder;
    const gchar *widget_id = NULL;
    GtkWidget   *widget    = NULL;

/*  eog_debug_message( DEBUG_PLUGINS, "key   = %s\n", key   );  */
/*  eog_debug_message( DEBUG_PLUGINS, "value = %s\n", value );  */
  
         IF_EQUAL(key,"Prompt"         ) { widget_id = "prompt_text_view";   }
    else IF_EQUAL(key,"Negative prompt") { widget_id = "negative_text_view"; }
    else IF_EQUAL(key,"Steps"          ) { widget_id = "steps_entry";        }
    else IF_EQUAL(key,"Sampler"        ) { widget_id = "sampler_entry";      }
    else IF_EQUAL(key,"CFG scale"      ) { widget_id = "cfg_scale_entry";    }
    else IF_EQUAL(key,"Seed"           ) { widget_id = "seed_entry";         }
    else IF_EQUAL(key,"Model hash"     ) { widget_id = "model_hash_entry";   }
    else IF_EQUAL(key,"Model"          ) { widget_id = "model_entry";        }
    else IF_EQUAL(key,"Size"           ) { show_size(plugin,value,-1);       }
    else                                { show_unknown(plugin,key,value,-1); }
    /* .... */
    
    if( widget_id ) {
        widget = get_widget( builder, widget_id );
    }
    if( widget )
    {
        set_widget_text( widget, value, -1 );
        show_group_ancestor( widget );
    }
#   undef IF_EQUAL
}

/*-------------------------------- EVENTS ---------------------------------*/

static void
on_parse_sd_parameter(const char *key,
                      int         key_size,
                      const char *value,
                      int         value_size,
                      void       *user_ptr,
                      int         user_int)
{
    SDPromptViewerPlugin *plugin = SDPROMPT_VIEWER_PLUGIN( user_ptr );
    gchar *k = g_strndup(key, key_size);
    gchar *v = g_strndup(value, value_size); 
    show_sd_parameter( plugin, k, v );
    g_free(k);
    g_free(v);
}

static void
on_png_text_chunk_loaded(gchar *text, gpointer user_ptr, int user_int) {
    SDPromptViewerPlugin *plugin = SDPROMPT_VIEWER_PLUGIN( user_ptr );
    
    if( text && text[0] ) {
        eog_debug_message( DEBUG_PLUGINS, "#PROMPT:\n\"%s\"", text);
        hide_all_widgets( plugin );
        parse_sd_parameters( text, -1,
                             on_parse_sd_parameter, user_ptr, user_int );
    }
}

/*
static void
on_jpg_text_loaded(gchar *text, gpointer data_ptr, int data_int) {
    
}
*/

static void
on_selection_changed(EogThumbView *view, SDPromptViewerPlugin *plugin) {
    GFile *file; EogImage *image;
    
    if( eog_thumb_view_get_n_selected( view ) == 0 ) { return; }
    image = eog_thumb_view_get_first_selected_image( view );
    file  = image ? eog_image_get_file( image ) : NULL;
    if( file ) {
        show_spinner( plugin );
        load_png_text_chunk(file, "parameters", 
                            on_png_text_chunk_loaded, plugin, 0);
    }
    if( image ) { g_object_unref(image); }
    if( file  ) { g_object_unref(file ); }
}


/*---------------------------- USER INTERFACE -----------------------------*/

static void
activate_user_interface( EogWindowActivatable *activatable )
{
    SDPromptViewerPlugin *plugin = SDPROMPT_VIEWER_PLUGIN (activatable);
    EogWindow *window = plugin->window;
    GSettings *settings;
    GtkWidget *thumbview;
    GtkWidget *sidebar;
    GError* error = NULL;

    settings  = g_settings_new( SDPROMPT_VIEWER_GSCHEMA_ID );
    thumbview = eog_window_get_thumb_view( window );
    sidebar   = eog_window_get_sidebar( window );
    
    plugin->thumbview = EOG_THUMB_VIEW( thumbview );
    plugin->selection_changed_id =
        g_signal_connect( G_OBJECT (thumbview),
                          "selection-changed",
                          G_CALLBACK( on_selection_changed ),
                          plugin );


    plugin->sidebar_builder = gtk_builder_new ();
    gtk_builder_set_translation_domain (plugin->sidebar_builder,
                        GETTEXT_PACKAGE);
    if (!gtk_builder_add_from_resource (plugin->sidebar_builder,
                        PLUGIN_UI, &error))
    {
        g_warning ("Couldn't load UI resource: %s", error->message);
        g_error_free (error);
    }
    plugin->gtkbuilder_widget = GTK_WIDGET (gtk_builder_get_object (plugin->sidebar_builder, "viewport1"));

    eog_sidebar_add_page( EOG_SIDEBAR (sidebar),
                          _("Stable Diffusion"),
                          plugin->gtkbuilder_widget );
    gtk_widget_show_all( plugin->gtkbuilder_widget );

    /*-- binding settings to plugins keys --*/
    g_settings_bind( settings, SETTINGS_FORCE_MINIMUM_WIDTH,
                     plugin, "force-minimum-width", G_SETTINGS_BIND_GET);
    g_settings_bind( settings, SETTINGS_MINIMUM_WIDTH,
                     plugin, "minimum-width", G_SETTINGS_BIND_GET);
    g_settings_bind( settings, SETTINGS_ALWAYS_ON_TOP,
                     plugin, "always-on-top", G_SETTINGS_BIND_GET);

    /*-- force update display for first time --*/
    on_selection_changed(plugin->thumbview, plugin);

    
    g_object_unref (settings);
}

static void
deactivate_user_interface( EogWindowActivatable *activatable )
{
    SDPromptViewerPlugin *plugin = SDPROMPT_VIEWER_PLUGIN (activatable);
    GtkWidget *sidebar, *thumbview;

    /* remove_statusbar_entry (plugin); */

    sidebar = eog_window_get_sidebar (plugin->window);
    eog_sidebar_remove_page(EOG_SIDEBAR (sidebar),
                plugin->gtkbuilder_widget);

    thumbview = eog_window_get_thumb_view (plugin->window);
    g_signal_handler_disconnect (thumbview, plugin->selection_changed_id);

    g_object_unref (plugin->sidebar_builder);
    plugin->sidebar_builder = NULL;
}

static void
eog_window_activatable_iface_init(EogWindowActivatableInterface *iface)
{
    iface->activate   = activate_user_interface;
    iface->deactivate = deactivate_user_interface;
}

/*========================= PLUGIN MAIN FUNCTION ==========================*/

G_MODULE_EXPORT void
peas_register_types(PeasObjectModule *module)
{
    sdprompt_viewer_plugin_register_type( G_TYPE_MODULE(module) );
    peas_object_module_register_extension_type(
        module,
        EOG_TYPE_WINDOW_ACTIVATABLE, /* <- the interface to implement */
        TYPE_SDPROMPT_VIEWER_PLUGIN  /* <- my object                  */
    );

    /* register the "Preferences" object */
    sdprompt_viewer_preferences_register_types( module );
}

