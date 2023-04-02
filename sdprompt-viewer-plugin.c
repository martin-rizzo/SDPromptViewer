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

/*---------------------------- USER INTERFACE -----------------------------*/

static void
hide_all_widgets( GtkBuilder *builder )
{
    GtkWidget *main_container = get_widget( builder, "main_container" );
    if( main_container ) { hide_group_descendants( main_container ); }
}

static void
show_widget( GtkBuilder  *builder,
             const gchar *widget_name,
             const gchar *str_value,
             const gchar *str_value_default)
{
    GtkWidget *widget = get_widget( builder, widget_name );
    if( !widget ) { return; }
    
    if( str_value ) {
        set_widget_text( widget, str_value, -1 );
        show_group_ancestor( widget );
    } else {
        set_widget_text( widget, str_value_default, -1 );
    }
}

static void
show_unknowns( GtkBuilder   *builder,
               SDParameters *parameters )
{
    int i;
    GtkTextView   *text_view = GTK_TEXT_VIEW( get_widget(builder, "unknown_text_view") );
    GtkTextBuffer *buffer    = text_view ? gtk_text_view_get_buffer( text_view ) : NULL;
    if( !buffer ) { return; }
    
    gtk_text_buffer_set_text( buffer, "", -1 );
    if( parameters->unknowns_count > 0 ) {
        show_group_ancestor( GTK_WIDGET(text_view) );
        for( i=0; i<parameters->unknowns_count; ++i ) {
            const char *key   = parameters->unknowns[i].key;
            const char *value = parameters->unknowns[i].value;
            gtk_text_buffer_insert_at_cursor( buffer,  key  , -1 );
            gtk_text_buffer_insert_at_cursor( buffer,  ": " , -1 );
            gtk_text_buffer_insert_at_cursor( buffer, value , -1 );
            gtk_text_buffer_insert_at_cursor( buffer, "\n"  , -1 );
        }        
    }
}

static void
show_spinner( SDPromptViewerPlugin *plugin )
{
    GtkBuilder *builder = plugin->sidebar_builder;
    GtkWidget  *spinner = builder ? get_widget(builder, "loading_spinner") : NULL;
    if( spinner ) {
        hide_all_widgets( builder );
        show_group_ancestor( spinner );
    }
}

static void
show_message( SDPromptViewerPlugin *plugin, const gchar *message )
{
    GtkBuilder *builder = plugin->sidebar_builder;
    GtkWidget  *label   = builder ? get_widget(builder, "message_label") : NULL;
    if( label ) {
        hide_all_widgets( builder );
        show_widget( builder, "message_label", message, "");
    }
}

static void
show_sd_parameters( SDPromptViewerPlugin *plugin,
                    void                 *buffer,
                    int                   buffer_size )
{
    GtkBuilder *b, *builder; GtkWidget *main_container;
    SDParameters parameters;
    
    builder        = plugin->sidebar_builder;
    main_container = builder ? get_widget( builder, "main_container" ) : NULL;
    if( !main_container ) { return; }
    
    parse_sd_parameters_from_buffer( &parameters, buffer, buffer_size );
    
    b = builder;
    hide_all_widgets( builder );    
    show_widget(b,"prompt_text_view"       ,parameters.prompt           ,"");
    show_widget(b,"negative_text_view"     ,parameters.negative_prompt  ,"");
    show_widget(b,"wildcard_text_view"     ,parameters.wildcard_prompt  ,"");
    show_widget(b,"model_entry"            ,parameters.model            ,"");
    show_widget(b,"model_hash_entry"       ,parameters.model_hash       ,"");
    show_widget(b,"sampler_entry"          ,parameters.sampler          ,"");
    show_widget(b,"steps_entry"            ,parameters.steps            ,"");
    show_widget(b,"cfg_scale_entry"        ,parameters.cfg_scale        ,"");
    show_widget(b,"seed_entry"             ,parameters.seed             ,"");
    show_widget(b,"width_entry"            ,parameters.width            ,"");
    show_widget(b,"height_entry"           ,parameters.height           ,"");
    show_widget(b,"hires_upscaler_entry"   ,parameters.hires_upscaler   ,"");
    show_widget(b,"hires_steps_entry"      ,parameters.hires_steps      ,"");
    show_widget(b,"hires_denoising_entry"  ,parameters.hires_denoising  ,"");
    show_widget(b,"hires_upscale_entry"    ,parameters.hires_upscale    ,"");
    show_widget(b,"hires_width_entry"      ,parameters.hires_width      ,"");
    show_widget(b,"hires_height_entry"     ,parameters.hires_height     ,"");
    show_widget(b,"inpaint_denoising_entry",parameters.inpaint_denoising,"");
    show_widget(b,"inpaint_mask_blur_entry",parameters.inpaint_mask_blur,"");
    show_widget(b,"eta_entry"              ,parameters.eta              ,"");
    show_widget(b,"ensd_entry"             ,parameters.ensd             ,"");
    show_widget(b,"clip_skip_entry"        ,parameters.clip_skip        ,"");
    show_unknowns( builder, &parameters );
    
    if( plugin->force_visibility ) {
        EogWindow *window  = plugin->window;
        GtkWidget *sidebar = window ? eog_window_get_sidebar( window ) : NULL;
        if( sidebar ) {
            eog_sidebar_set_page( EOG_SIDEBAR(sidebar), plugin->gtkbuilder_widget );
        }
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

/*-------------------------------- EVENTS ---------------------------------*/

static void
on_png_text_chunk_loaded(gchar *text, gpointer user_ptr, int user_int) {
    SDPromptViewerPlugin *plugin = SDPROMPT_VIEWER_PLUGIN( user_ptr );
    
    if( text && text[0] ) {
        show_sd_parameters( plugin, text, -1 );
    } else {
        show_message( plugin,
        "No Stable Diffusion parameters found in the image." );
    }
}

/*
static void
on_jpg_text_file_loaded(gchar *text, gpointer user_ptr, int user_int) {
    
}
*/

static void
on_selection_changed(EogThumbView *view, SDPromptViewerPlugin *plugin) {
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
on_activate( EogWindowActivatable *activatable )
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
                          _("Stable Diffusion Parameters"),
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

    /*-- force update display for first time --*/
    on_selection_changed(plugin->thumbview, plugin);

    
    g_object_unref (settings);
}

static void
on_deactivate( EogWindowActivatable *activatable )
{
    SDPromptViewerPlugin *plugin = SDPROMPT_VIEWER_PLUGIN (activatable);
    GtkWidget *sidebar, *thumbview;

    set_sidebar_minimum_width( plugin, -1 );

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
    iface->activate   = on_activate;
    iface->deactivate = on_deactivate;
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

