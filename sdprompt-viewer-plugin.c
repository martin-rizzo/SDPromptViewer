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
#include "eog/eog-window.h"
#include "gtk/gtkcssprovider.h"
#include "libpeas-gtk/peas-gtk-configurable.h"
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
#include "utils_widget.h"
#include "utils_sdparams.h"
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
    PROP_SHOW_UNKNOWN_PARAMS,
    PROP_FORCE_MINIMUM_WIDTH,
    PROP_MINIMUM_WIDTH,
    PROP_FORCE_VISIBILITY,
    NUMBER_OF_PROPS
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
    GParamFlags flags;
    GObjectClass *object_class = G_OBJECT_CLASS( klass );
    object_class->dispose      = sdprompt_viewer_plugin_dispose;

    /* register properties */
    object_class->set_property = sdprompt_viewer_plugin_set_property;
    object_class->get_property = sdprompt_viewer_plugin_get_property;
    g_object_class_override_property( object_class, PROP_WINDOW, "window" );
    
    flags = ( G_PARAM_READWRITE | G_PARAM_STATIC_NAME );
    
    g_object_class_install_property(
        object_class, PROP_SHOW_UNKNOWN_PARAMS,
        g_param_spec_boolean("show-unknown-params",0,0, FALSE, flags) );
    
    g_object_class_install_property(
        object_class, PROP_FORCE_MINIMUM_WIDTH,
        g_param_spec_boolean("force-minimum-width",0,0, FALSE, flags) );
    
    g_object_class_install_property(
        object_class, PROP_MINIMUM_WIDTH,
        g_param_spec_double("minimum-width",0,0, 100,1000, 480, flags) );
                                      
    g_object_class_install_property(
        object_class, PROP_FORCE_VISIBILITY,
        g_param_spec_boolean("force-visibility",0,0, FALSE, flags) );
    
}

static void
sdprompt_viewer_plugin_class_finalize( SDPromptViewerPluginClass *klass )
{
    /* needed for G_DEFINE_DYNAMIC_TYPE_EXTENDED */
}

static void
sdprompt_viewer_plugin_init( SDPromptViewerPlugin *plugin )
{
    eog_debug_message( DEBUG_PLUGINS, "SDPromptViewerPlugin initializing" );
    plugin->sidebar_min_stored = FALSE;
}

static void
sdprompt_viewer_plugin_dispose( GObject *object )
{
    SDPromptViewerPlugin *plugin = SDPROMPT_VIEWER_PLUGIN( object );
    eog_debug_message( DEBUG_PLUGINS, "SDPromptViewerPlugin disposing" );
    
    g_clear_object( &plugin->window );

    G_OBJECT_CLASS( sdprompt_viewer_plugin_parent_class )->dispose( object );
}

/*--------------------  ---------------------*/

static void
set_image_generation_data( SDPromptViewerPlugin *plugin,
                           const gchar          *data,
                           int                   max_bytes )
{
    if( plugin->image_generation_data ) {
        g_free( plugin->image_generation_data );
        plugin->image_generation_data = NULL;
    }
    if( max_bytes!=0 && data!=NULL && data[0]!='\0' ) {
        if( max_bytes<0 ) { max_bytes = strlen( data ); }
        plugin->image_generation_data   = g_strndup( data, max_bytes );
        plugin->image_generation_length = max_bytes;
    }
}

static void
set_sidebar_minimum_width( SDPromptViewerPlugin *plugin,
                           gint                  min_width)
{
    EogWindow *window  = plugin->window;
    GtkWidget *sidebar = window ? eog_window_get_sidebar( window ) : NULL;
    if( !sidebar ) { return; }

    /* store original minimum size */
    if( min_width >= 0 ) {
        if( !plugin->sidebar_min_stored ) {
             plugin->sidebar_min_stored = TRUE;
            gtk_widget_get_size_request(
                sidebar, &plugin->sidebar_min_width, &plugin->sidebar_min_height );
        }
    }
    /* restore original minimum size */
    else {
        if( plugin->sidebar_min_stored ) {
            plugin->sidebar_min_stored = FALSE;
            gtk_widget_set_size_request(
                sidebar, plugin->sidebar_min_width, plugin->sidebar_min_height );
        }
    }
    /* if the original minimum size is stored then apply the new one */
    if( plugin->sidebar_min_stored ) {
        gtk_widget_set_size_request( sidebar, min_width, plugin->sidebar_min_height );
    }
}

/*-------------------- CONTROLLING THE USER INTERFACE ---------------------*/

static void
hide_all_widgets( GtkBuilder  *builder )
{
    GtkWidget *main_container;
    GList *children, *iter; GtkWidget *child;
    
    main_container = builder ? get_widget( builder, "main_container" ) : NULL;
    if( main_container ) {
        children = gtk_container_get_children( GTK_CONTAINER(main_container) );
        for( iter = children ; iter ; iter = g_list_next(iter) ) {
            child = GTK_WIDGET( iter->data );
            gtk_widget_hide( child );
        }
    }
}

static void
show_widget( GtkBuilder  *builder, 
             const gchar *widget_name,
             gboolean     show )
{
    GtkWidget *widget;
    widget = builder ? get_widget( builder, widget_name ) : NULL;
    if( widget ) {
        if( show ) { gtk_widget_show( widget ); }
        else       { gtk_widget_hide( widget ); }
    }
}

static void
show_spinner( SDPromptViewerPlugin *plugin )
{
    GtkBuilder *builder = plugin->sidebar_builder;
    if( !builder ) { return; }
    hide_all_widgets( builder );
    show_widget( builder, "loading_group", TRUE );
}

static void
show_message( SDPromptViewerPlugin *plugin,
              const gchar          *message )
{
    GtkBuilder *builder = plugin->sidebar_builder;
    if( !builder ) { return; }
    hide_all_widgets( builder );
    display_text( builder , "message_label", message );
    show_widget( builder  , "message_group", TRUE    );
}

static void
show_image_generation_data( SDPromptViewerPlugin *plugin )
{
    SDParameters parameters;
    GtkTextView *text_view; GtkTextBuffer *buffer; int i;
    GtkBuilder *b = plugin->sidebar_builder;
    if( !b ) { return; }
        
    /* If no generation data is present, show a message and return */
    if( !plugin->image_generation_data ) {
        show_message( plugin,
                      "No Stable Diffusion parameters found in the image." );
        return;
    }
    
    parse_sd_parameters_from_buffer( &parameters,
                                     plugin->image_generation_data,
                                     plugin->image_generation_length );
    
    hide_all_widgets( b );
    display_text(b, "prompt_text_view"       , parameters.prompt            );
    display_text(b, "negative_text_view"     , parameters.negative_prompt   );
    display_text(b, "wildcard_text_view"     , parameters.wildcard_prompt   );
    display_text(b, "model_entry"            , parameters.model.name        );
    display_text(b, "model_hash_entry"       , parameters.model.hash        );
    display_text(b, "sampler_entry"          , parameters.sampler           );
    display_text(b, "steps_entry"            , parameters.steps             );
    display_text(b, "cfg_scale_entry"        , parameters.cfg_scale         );
    display_text(b, "seed_entry"             , parameters.seed              );
    display_text(b, "width_entry"            , parameters.width             );
    display_text(b, "height_entry"           , parameters.height            );
    display_text(b, "hires_upscaler_entry"   , parameters.hires.upscaler    );
    display_text(b, "hires_steps_entry"      , parameters.hires.steps       );
    display_text(b, "hires_denoising_entry"  , parameters.hires.denoising   );    
    display_text(b, "inpaint_denoising_entry", parameters.inpaint.denoising );
    display_text(b, "inpaint_mask_blur_entry", parameters.inpaint.mask_blur );
    
    display_text_box(b, "eta_box"      , parameters.settings.eta       );
    display_text_box(b, "ensd_box"     , parameters.settings.ensd      );
    display_text_box(b, "clip_skip_box", parameters.settings.clip_skip );
    
    display_text_or_float(b, "hires_width_entry",
                      parameters.hires.width,
                      parameters.hires.calc_width,0);
    display_text_or_float(b, "hires_height_entry",
                      parameters.hires.height,
                      parameters.hires.calc_height,0);
    display_text_or_float(b, "hires_upscale_entry",
                      parameters.hires.upscale,
                      parameters.hires.calc_upscale,2);
    
    text_view = GTK_TEXT_VIEW( get_widget(b, "unknown_text_view") );
    buffer    = text_view ? gtk_text_view_get_buffer( text_view ) : NULL;
    if( buffer ) {
        show_widget( b, "unknown_group", parameters.unknowns_count > 0 );
        gtk_text_buffer_set_text( buffer, "", -1 );
        for( i=0; i<parameters.unknowns_count; ++i ) {
            const char *key   = parameters.unknowns[i].key;
            const char *value = parameters.unknowns[i].value;
            gtk_text_buffer_insert_at_cursor( buffer,  key  , -1 );
            gtk_text_buffer_insert_at_cursor( buffer,  ": " , -1 );
            gtk_text_buffer_insert_at_cursor( buffer, value , -1 );
            gtk_text_buffer_insert_at_cursor( buffer, "\n"  , -1 );
        }        
    }
    
    show_widget(b, "buttons_group"   , TRUE                             );
    show_widget(b, "prompt_group"    , parameters.prompt!=NULL          );
    show_widget(b, "negative_group"  , parameters.negative_prompt!=NULL ); 
    show_widget(b, "wildcard_group"  , parameters.wildcard_prompt!=NULL );
    show_widget(b, "parameters_group", TRUE                             );
    show_widget(b, "model_group"     , parameters.model.has_info        ); 
    show_widget(b, "hires_group"     , parameters.hires.has_info        );
    show_widget(b, "inpaint_group"   , parameters.inpaint.has_info      );
    show_widget(b, "settings_group"  , parameters.settings.has_info     );
    
    if( plugin->force_visibility ) {
        EogWindow *window  = plugin->window;
        GtkWidget *sidebar = window ? eog_window_get_sidebar( window ) : NULL;
        if( sidebar ) {
            eog_sidebar_set_page( EOG_SIDEBAR(sidebar), plugin->gtkbuilder_widget );
        }
    }
}

/*-------------------------------- EVENTS ---------------------------------*/

static void
on_png_text_chunk_loaded(gchar *text, gpointer user_ptr, int user_int) {
    SDPromptViewerPlugin *plugin = SDPROMPT_VIEWER_PLUGIN( user_ptr );
    set_image_generation_data( plugin, text, -1 );
    show_image_generation_data( plugin );
}

/*
static void
on_jpg_text_file_loaded(gchar *text, gpointer user_ptr, int user_int) {
    
}
*/

static void
on_selection_changed( EogThumbView *view, SDPromptViewerPlugin *plugin ) {
    GFile *file; EogImage *image;
    
    if( eog_thumb_view_get_n_selected( view ) == 0 ) {
        show_message( plugin, "No image selected." );
        return;
    }
    image = eog_thumb_view_get_first_selected_image( view );
    file  = image ? eog_image_get_file( image ) : NULL;
    if( file ) {
        show_spinner( plugin );
        load_png_text_chunk(file, "parameters", 
                            on_png_text_chunk_loaded, plugin, 0);
    }
    if( file  ) { g_object_unref(file ); }
    if( image ) { g_object_unref(image); }
}

static void
on_copy_data_clicked( GtkWidget *widget, gpointer data ) {
    SDPromptViewerPlugin *plugin = SDPROMPT_VIEWER_PLUGIN( data );
    
    if( plugin->image_generation_data != NULL ) {
        gtk_clipboard_set_text(
            gtk_clipboard_get( GDK_SELECTION_CLIPBOARD ),
            plugin->image_generation_data,
            plugin->image_generation_length );
    }
}

static void
on_preferences_clicked( GtkWidget *widget, gpointer data ) {
    
    SDPromptViewerPreferences *preferences =
        g_object_new( TYPE_SDPROMPT_VIEWER_PREFERENCES, NULL);
        
    GtkWidget *preferences_widget =
      peas_gtk_configurable_create_configure_widget( PEAS_GTK_CONFIGURABLE( preferences ) );

    /* create a Gtk dialog that contains the plugin's configuration widget */
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Plugin Configuration",
                                                     NULL,
                                                     GTK_DIALOG_MODAL,
                                                     "Close",
                                                     GTK_RESPONSE_CLOSE,
                                                     NULL);
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), preferences_widget);
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_object_unref( preferences_widget );
    g_object_unref( preferences );
}

static void
on_activate( EogWindowActivatable *activatable )
{
    SDPromptViewerPlugin *plugin = SDPROMPT_VIEWER_PLUGIN (activatable);
    EogWindow *window = plugin->window;
    GSettings *settings;
    GtkWidget *thumbview;
    GtkWidget *sidebar;
    GdkScreen *screen;
    GtkWidget *button;
    GtkBuilder *builder;
    GError* error = NULL;
    
    /*-- add CSS styles --*/
    plugin->css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_resource(
        plugin->css_provider, RES_DEFAULT_CSS);
    screen = gdk_screen_get_default();
    if( screen ) {
        gtk_style_context_add_provider_for_screen(
            screen, GTK_STYLE_PROVIDER( plugin->css_provider ),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION );
    }
    
    settings  = g_settings_new( SDPROMPT_VIEWER_GSCHEMA_ID );
    thumbview = eog_window_get_thumb_view( window );
    sidebar   = eog_window_get_sidebar( window );
    
    plugin->thumbview = EOG_THUMB_VIEW( thumbview );


    /*-- build the user interface --*/
    plugin->sidebar_builder = builder = gtk_builder_new();
    gtk_builder_set_translation_domain( plugin->sidebar_builder,
                                        GETTEXT_PACKAGE );
    if( !gtk_builder_add_from_resource( plugin->sidebar_builder,
                                        RES_PLUGIN_UI,
                                        &error) ) {
        g_warning( "Couldn't load UI resource: %s", error->message );
        g_error_free( error );
    }
    plugin->gtkbuilder_widget = get_widget( plugin->sidebar_builder, "viewport1" );

    /*-- add the user interface to the sidebar */
    eog_sidebar_add_page( EOG_SIDEBAR( sidebar ),
                          _("Stable Diffusion Prompt Viewer"),
                          plugin->gtkbuilder_widget );
    gtk_widget_show_all( plugin->gtkbuilder_widget );

    /*-- binding configurable properties --*/
    g_settings_bind( settings, SETTINGS_SHOW_UNKNOWN_PARAMS,
                     plugin, "show-unknown-params", G_SETTINGS_BIND_GET);
    g_settings_bind( settings, SETTINGS_FORCE_MINIMUM_WIDTH,
                     plugin, "force-minimum-width", G_SETTINGS_BIND_GET);
    g_settings_bind( settings, SETTINGS_MINIMUM_WIDTH,
                     plugin, "minimum-width", G_SETTINGS_BIND_GET);
    g_settings_bind( settings, SETTINGS_FORCE_VISIBILITY,
                     plugin, "force-visibility", G_SETTINGS_BIND_GET);
    
    /*-- binding events using signals --*/
    plugin->selection_changed_id =
        g_signal_connect( G_OBJECT( thumbview ),
                          "selection-changed",
                          G_CALLBACK( on_selection_changed ),
                          plugin );

    button = get_widget( builder, "preferences_button" );
    plugin->preferences_clicked_id =
        g_signal_connect( G_OBJECT( button ),
                          "clicked",
                          G_CALLBACK( on_preferences_clicked ),
                          plugin );
        
    button = get_widget( builder, "copy_data_button" );
    plugin->copy_data_clicked_id =
        g_signal_connect( G_OBJECT( button ),
                          "clicked",
                          G_CALLBACK( on_copy_data_clicked ),
                          plugin );

    /*-- force update display for first time --*/
    on_selection_changed(plugin->thumbview, plugin);

    /*-- clean up --*/
    if( settings ) { g_object_unref( settings ); }
    
}

static void
on_deactivate( EogWindowActivatable *activatable )
{
    SDPromptViewerPlugin *plugin = SDPROMPT_VIEWER_PLUGIN (activatable);
    GtkWidget *sidebar, *thumbview;
    GdkScreen *screen;
    GtkWidget *button;
    GtkBuilder *builder = plugin->sidebar_builder;

    /*-- restore sidebar width & release stored data --*/
    set_sidebar_minimum_width( plugin, -1 );
    set_image_generation_data( plugin, NULL, 0 );

    /*-- remove CSS styles --*/
    screen = gdk_screen_get_default();
    if( screen ) {
        gtk_style_context_remove_provider_for_screen(
            screen, GTK_STYLE_PROVIDER(plugin->css_provider) );
    }
    

    /*-- remove the user interface from the sidebar --*/
    sidebar = eog_window_get_sidebar( plugin->window );
    eog_sidebar_remove_page( EOG_SIDEBAR( sidebar ),
                             plugin->gtkbuilder_widget );

    /*-- remove signals --*/
    thumbview = eog_window_get_thumb_view( plugin->window );
    g_signal_handler_disconnect( thumbview, plugin->selection_changed_id );
    button = get_widget( builder, "preferences_button" );
    g_signal_handler_disconnect( button, plugin->preferences_clicked_id );
    button = get_widget( builder, "copy_data_button" );
    g_signal_handler_disconnect( button, plugin->copy_data_clicked_id );
    
    if( plugin->sidebar_builder ) {
        g_object_unref( plugin->sidebar_builder );
        plugin->sidebar_builder = NULL;
    }
    if( plugin->css_provider ) {
        g_object_unref( plugin->css_provider );
        plugin->css_provider = NULL;
    }
}

static void
eog_window_activatable_iface_init(EogWindowActivatableInterface *iface)
{
    iface->activate   = on_activate;
    iface->deactivate = on_deactivate;
}

/*------------------------------ PROPERTIES -------------------------------*/

static void
sdprompt_viewer_plugin_set_property(GObject       *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
    SDPromptViewerPlugin *plugin = SDPROMPT_VIEWER_PLUGIN (object);
    switch (prop_id)
    {
        case PROP_SHOW_UNKNOWN_PARAMS:
            plugin->show_unknown_params = g_value_get_boolean(value);
            break;
            
        case PROP_FORCE_MINIMUM_WIDTH:
            plugin->force_minimum_width = g_value_get_boolean(value);
            set_sidebar_minimum_width( plugin,
                plugin->force_minimum_width ? (gint)plugin->minimum_width : -1 );
            break;
            
        case PROP_MINIMUM_WIDTH:
            plugin->minimum_width = g_value_get_double(value);
            set_sidebar_minimum_width( plugin,
                plugin->force_minimum_width ? (gint)plugin->minimum_width : -1 );
            break;
            
        case PROP_FORCE_VISIBILITY:
            plugin->force_visibility = g_value_get_boolean(value);
            break;

        case PROP_WINDOW:
            plugin->window = EOG_WINDOW( g_value_dup_object(value) );
            break;
            
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
sdprompt_viewer_plugin_get_property(GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
    SDPromptViewerPlugin *plugin = SDPROMPT_VIEWER_PLUGIN( object );
    switch (prop_id)
    {
        case PROP_SHOW_UNKNOWN_PARAMS:
            g_value_set_boolean(value, plugin->show_unknown_params);
            break;
            
        case PROP_FORCE_MINIMUM_WIDTH:
            g_value_set_boolean(value, plugin->force_minimum_width);
            break;
            
        case PROP_MINIMUM_WIDTH:
            g_value_set_double(value, plugin->minimum_width);
            break;
            
        case PROP_FORCE_VISIBILITY:
            g_value_set_boolean(value, plugin->force_visibility);
            break;
            
        case PROP_WINDOW:
            g_value_set_object(value, plugin->window);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
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

