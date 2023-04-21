/**
 * @file    themes.h
 * @brief   Provides a basic functionality for creating themes for the plugin.
 * @author  Martin Rizzo | <martinrizzo@gmail.com>
 * @date    Apr 19, 2023
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
#include <gtk/gtk.h>
#if !defined( THEMES_RES_DIR )
#  define THEMES_RES_DIR ""
#  error "THEMES_RES_DIR is not defined. \
         (It's possible that you forgot to include resources.h?)"
#endif


/**
 * THEME_STYLE_TYPE:
 * 
 * @THEME_VISUAL_STYLE: Specifies a visual style for the widgets. This
 *    includes colors, formatting, padding, and is often referred as a "skin".
 * @THEME_BORDER_STYLE: Specifies the border style for widgets that display
 *    data. This determines whether or not to include borders and their width.
 * @THEME_ZOOM_STYLE: Specifies the font size for the widgets. This can be
 *    small, normal, or large.
 *
 * An enumeration that specifies the type of style to create.
 */
typedef enum _THEME_STYLE_TYPE THEME_STYLE_TYPE;
enum         _THEME_STYLE_TYPE {
    THEME_VISUAL_STYLE,
    THEME_BORDER_STYLE,
    THEME_ZOOM_STYLE
};

/**
 * new_theme_style_provider - Creates a GtkStyleProvider based on type and ID.
 * 
 * @style_type: The type of style to create.
 * @style_id: The ID of the style to create.
 *
 * The @style_type parameter specifies the type of style to create.
 * There are three types of styles:
 *   - THEME_VISUAL_STYLE: Specifies a visual style for the widgets.
 *                         (colors, formatting, padding, ...)
 *   - THEME_BORDER_STYLE: Specifies the border style for data widgets.
 *                         (whether or not to include borders and its width)
 *   - THEME_ZOOM_STYLE:   Specifies the font size for the widgets.
 *                         (small, normal, largo, ...)
 *
 * Returns: (transfer full): A new #GtkStyleProvider,
 *                           or %NULL if an error occurred.
 */
static GtkStyleProvider *
new_theme_style_provider( THEME_STYLE_TYPE style_type,
                          gint             style_id )
{
    int i; GtkCssProvider *css_provider; const gchar **resources;
    
    static const gchar *visual_style_resources[] =
    {
        THEMES_RES_DIR"/vs_none.css",
        THEMES_RES_DIR"/vs_autumn_twilight.css",
        THEMES_RES_DIR"/vs_frosty_dawn.css",
        NULL
    };
    static const gchar *border_style_resources[] =
    {
        THEMES_RES_DIR"/bs_none.css",
        THEMES_RES_DIR"/bs_line.css",
        THEMES_RES_DIR"/bs_thick.css",
        NULL
    };
    static const gchar *zoom_style_resources[] =
    {
        THEMES_RES_DIR"/fs_xsmall.css",
        THEMES_RES_DIR"/fs_small.css",
        THEMES_RES_DIR"/fs_medium.css",
        THEMES_RES_DIR"/fs_large.css",
        THEMES_RES_DIR"/fs_xlarge.css",
        NULL
    };
    
    switch( style_type ) {
        default:
        case THEME_VISUAL_STYLE: resources = visual_style_resources; break;
        case THEME_BORDER_STYLE: resources = border_style_resources; break;
        case THEME_ZOOM_STYLE:   resources = zoom_style_resources; style_id+=2; break;
    }
    /* verify if 'style_id' is valid */
    if( style_id<0 ) { return NULL; }
    for( i=0; i<=style_id; i++) {
        if( resources[i]==NULL ) { return NULL; }
    }
    /* create the provider and load the css info into it */
    css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_resource( css_provider,
                                         resources[style_id] );
    return GTK_STYLE_PROVIDER( css_provider );
}

