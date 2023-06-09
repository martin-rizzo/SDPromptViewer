/**
 * @file    sdprompt-viewer-plugin.h
 * @brief   Declares the main user interface for the plugin.
 * @author  Martin Rizzo | <martinrizzo@gmail.com>
 * @date    Mar 25, 2023
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
#ifndef __SDPROMPT_VIEWER_PLUGIN_H__
#define __SDPROMPT_VIEWER_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>
#include <eog/eog-thumb-view.h>
#include <eog/eog-sidebar.h>
#include <eog/eog-window.h>
typedef struct SDPromptTheme_ SDPromptTheme;
struct         SDPromptTheme_ {
    gint visual_style;
    gint border_size;
    gint font_size;
};
G_BEGIN_DECLS

#define TYPE_SDPROMPT_VIEWER_PLUGIN sdprompt_viewer_plugin_get_type()

/*------------------- TYPE CHECKING AND CASTING MACROS --------------------*/

/**
 * Macro to cast any #GObject to the custom #SDPromptViewerPlugin object.
 * Returns: A pointer to the casted #SDPromptViewerPlugin instance.
 **/
#define SDPROMPT_VIEWER_PLUGIN(obj) (                                   \
    G_TYPE_CHECK_INSTANCE_CAST(                                         \
        (obj), TYPE_SDPROMPT_VIEWER_PLUGIN, SDPromptViewerPlugin )      \
)
/**
 * Macro to cast #GObjectClass to the custom #SDPromptViewerPluginClass type
 * Returns: A pointer to the casted #SDPromptViewerPluginClass instance.
 **/
#define SDPROMPT_VIEWER_PLUGIN_CLASS(klass) (                            \
    G_TYPE_CHECK_CLASS_CAST(                                             \
        (klass), TYPE_SDPROMPT_VIEWER_PLUGIN, SDPromptViewerPluginClass )\
)
/**
 * Macro to retrieve the class structure of a #SDPromptViewerPlugin instance.
 * Returns: A pointer to the #SDPromptViewerPluginClass structure of @obj.
 **/
#define SDPROMPT_VIEWER_PLUGIN_GET_CLASS(obj) (                          \
    G_TYPE_INSTANCE_GET_CLASS(                                           \
        (obj), TYPE_SDPROMPT_VIEWER_PLUGIN, SDPromptViewerPluginClass )  \
)
#define IS_SDPROMPT_VIEWER_PLUGIN(obj) (                                 \
    G_TYPE_CHECK_INSTANCE_TYPE( (obj), TYPE_SDPROMPT_VIEWER_PLUGIN )     \
)
#define IS_SDPROMPT_VIEWER_PLUGIN_CLASS(klass) (                         \
    G_TYPE_CHECK_CLASS_TYPE( (klass), TYPE_SDPROMPT_VIEWER_PLUGIN )      \
)

/*----------------------------- PLUGIN CLASS ------------------------------*/

typedef struct _SDPromptViewerPluginClass SDPromptViewerPluginClass;
struct         _SDPromptViewerPluginClass
{
    PeasExtensionBaseClass parent_class;
    
    gint instance_count;
    
    /* Visual Styles */
    SDPromptTheme    current_theme;
    gboolean         is_theme_locked;
    GtkStyleProvider *visual_style_provider;
    GtkStyleProvider *border_style_provider;
    GtkStyleProvider *zoom_style_provider;
    
    /* Minimum Sidebar Size */
    gint sidebar_min_width;
    gint sidebar_original_min_width;
    gint sidebar_original_min_height;
};

/*----------------------------- PLUGIN OBJECT -----------------------------*/

typedef struct _SDPromptViewerPlugin SDPromptViewerPlugin;
struct         _SDPromptViewerPlugin
{
    PeasExtensionBase parent_instance;

    EogWindow     *window;
    EogThumbView  *thumbview;
    EogSidebar    *sidebar;
    GtkWidget     *page;
    GtkBuilder    *page_builder;
    
    /* Properties */
    gboolean      show_unknown_params;
    gboolean      force_minimum_width;
    gdouble       minimum_width;
    gboolean      force_visibility;
    gchar        *image_generation_data;
    SDPromptTheme theme;

    /* Signal IDs */
    gulong thumbview_sel_changed_signal_id;
    gulong preferences_button_signal_id;
    gulong copy_button_signal_id;
    
    /* Minimum Sidebar Size */
    gboolean sidebar_min_is_forced;
    gint     sidebar_min_width;
};

/*---------------------------- PUBLIC FUNCTIONS ---------------------------*/

GType sdprompt_viewer_plugin_get_type(void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT void peas_register_types( PeasObjectModule *module );


G_END_DECLS
#endif /* __SDPROMPT_VIEWER_PLUGIN_H__ */
