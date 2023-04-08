/**
 * @file    utils_widget.h
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
#include <gtk/gtk.h>

/**
 * ensure_valid_utf8 - Converts a string to valid UTF-8 format.
 * @text:      a string of possibly invalid utf8 text.
 * @max_bytes: the max bytes to convert, or -1 to go until NUL.
 *
 * This function verifies whether the string is a valid UTF8 string.
 * If it is not valid, it attempts to convert the string to UTF8 using
 * the most appropriate method.
 * 
 * This is useful for ensuring that text is properly encoded before
 * being processed or displayed in a GTK-based application.
 *
 * Returns: a pointer to the string in valid UTF-8 format.
 *    The caller of the function takes ownership of the data,
 *    and is responsible for freeing it.
 */
static gchar *
ensure_valid_utf8( const char *text, int max_bytes ) {
    gchar *utf8_text = NULL;
    if( !g_utf8_validate(text, max_bytes, NULL) ) {
        utf8_text = g_convert( text, max_bytes, "UTF-8", "ISO-8859-1",
                               NULL,NULL,NULL );
    }
    return utf8_text    ? utf8_text :
           max_bytes>=0 ? g_strndup( text, max_bytes ) : g_strdup( text );
}

static void
set_widget_text_(GtkWidget *widget, const char *text, int max_bytes, int depth) {
    GtkTextBuffer *buffer; gchar *utf8_text;
    
    if( GTK_IS_LABEL(widget) && depth==0 ) {
        utf8_text = ensure_valid_utf8( text, max_bytes );
        gtk_label_set_text( GTK_LABEL(widget), utf8_text );
        g_free( utf8_text );
    }
    else if( GTK_IS_ENTRY(widget) ) {
        utf8_text = ensure_valid_utf8( text, max_bytes );
        gtk_entry_set_text( GTK_ENTRY(widget), utf8_text );
        g_free( utf8_text );
    }
    else if( GTK_IS_TEXT_VIEW(widget) ) {
        utf8_text = ensure_valid_utf8( text, max_bytes );
        buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW(widget) );
        if( buffer ) { gtk_text_buffer_set_text( buffer, utf8_text, -1 ); }
        g_free( utf8_text );
    }    
    else if( GTK_IS_CONTAINER(widget) ) {
        GList *children, *iter; GtkWidget *child;
        children = gtk_container_get_children( GTK_CONTAINER(widget) );
        for( iter = children ; iter ; iter = g_list_next(iter) ) {
            child = GTK_WIDGET( iter->data );
            set_widget_text_( child, text, max_bytes, depth+1 );
        }
    }
}

/**
 * set_widget_text - Sets the text of a widget.
 * @widget:    The widget to set the text of.
 * @text:      A str of possibly invalid utf8 text to set as the widget's text.
 * @max_bytes: The maximum number of bytes in @text, or -1 to go until NUL.
 *
 * This function sets the text of the specified widget to the provided @text.
 * If @max_bytes is -1, the text is assumed to be NUL-terminated. If the
 * provided text contains invalid UTF-8 sequences, it will be automatically
 * converted to a valid UTF-8 string using the most appropriate method.
 */
static void
set_widget_text(GtkWidget *widget, const char *text, int max_bytes) {
    set_widget_text_( widget, text, max_bytes, 0 );
}

/**
 * clear_widget_text - Clears the text content of a widget.
 * @widget: the #GtkWidget to clear the text content of.
 */
static void
clear_widget_text(GtkWidget *widget) {
    GtkTextBuffer *buffer;
    if( GTK_IS_LABEL(widget) ) {
        gtk_label_set_text( GTK_LABEL(widget), "");
    }
    else if( GTK_IS_ENTRY(widget) ) {
        gtk_entry_set_text( GTK_ENTRY(widget), "" );
    }
    else if( GTK_IS_TEXT_VIEW(widget) ) {
        buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW(widget) );
        gtk_text_buffer_set_text( buffer, "", -1 );
    }
}

static void
clear_descendants_text(GtkWidget *widget, gboolean include_labels) {
    GList *children, *iter; GtkWidget *child; GtkTextBuffer *buffer;
    if( GTK_IS_CONTAINER(widget) ) {
        children = gtk_container_get_children( GTK_CONTAINER(widget) );
        for( iter = children ; iter ; iter = g_list_next(iter) ) {
            child = GTK_WIDGET( iter->data );
            if( GTK_IS_LABEL(child) ) {
                if( include_labels ) {
                    gtk_label_set_text( GTK_LABEL(child), "" );
                }
            }
            else if( GTK_IS_ENTRY(child) ) {
                gtk_entry_set_text( GTK_ENTRY(child), "" );
            }
            else if( GTK_IS_TEXT_VIEW(child) ) {
                buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW(child) );
                if( buffer ) { gtk_text_buffer_set_text( buffer, "", -1 ); }
            }
            else {
                clear_descendants_text( child, include_labels );
            }
        }
        g_list_free( children );
    }    
}


/**
 * show_group_ancestor - Make visible the ancestor whose ID ends with "_group".
 * @widget: The starting widget to search from.
 */
static void
show_group_ancestor(GtkWidget *widget) {
    GtkBuildable *buildable; GtkWidget *parent; const gchar *parent_id;
    parent = widget;
    while( parent ) {
        buildable = GTK_BUILDABLE( parent );
        if( buildable ) {
            parent_id = gtk_buildable_get_name( buildable );
            if( g_str_has_suffix(parent_id, "_group") ) {
                gtk_widget_show( parent );
                return;
            }
        }
        parent = gtk_widget_get_parent( parent );
    }
}

/**
 * hide_descendants - Recursively hides widget children matching name suffix
 * @widget: A widget to hide children from.
 * @suffix: A suffix of the name of the children to hide or %NULL
 *          to hide all children.
 *
 * This function is used to hide the children of a container widget that
 * have a name suffix matching the provided @suffix. If @suffix is NULL,
 * all children will be hidden. The function is recursive and will hide
 * all descendants of the container widget that match the suffix.
 */
static void
hide_descendants(GtkWidget *widget, const gchar *suffix) {
    GList *children, *iter;
    GtkBuildable *buildable; GtkWidget *child; const gchar *child_id;
    if( GTK_IS_CONTAINER(widget) ) {
        children = gtk_container_get_children( GTK_CONTAINER(widget) );
        for( iter = children ; iter ; iter = g_list_next(iter) ) {
            child     = GTK_WIDGET( iter->data );
            buildable = GTK_BUILDABLE( child );
            if( buildable ) {
                child_id = gtk_buildable_get_name( buildable );
                if( suffix==NULL || g_str_has_suffix( child_id, suffix ) ) {
                    gtk_widget_hide( child );
                }
                hide_descendants( child, suffix );
            }
        }
        g_list_free( children );
    }
}


/**
 * get_widget - Retrieves a widget with the specified name.
 * @builder:     a GtkBuilder object
 * @widget_name: the name of the widget to retrieve from the builder
 *
 * Returns: a GtkWidget pointer to the requested widget.
 **/
#define get_widget(builder, widget_name) \
    GTK_WIDGET( gtk_builder_get_object( builder, widget_name ) )

