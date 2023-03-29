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

/**
 * ensure_valid_utf8 - Converts a string to valid UTF-8 format.
 * @text:    a string of possibly invalid utf8 text
 * @max_len: max bytes to convert, or -1 to go until NUL.
 *
 * This function verifies whether the string is a valid UTF8 string.
 * If it is not valid, it attempts to convert the string to UTF8 using
 * the most appropriate method.
 * 
 * This is useful for ensuring that text is properly encoded before
 * being processed or displayed in a GTK-based application.
 *
 * Returns: a pointer to the string in valid UTF-8 format.
 */
static gchar *
ensure_valid_utf8( const char *text, int len )
{
    gchar *utf8_text = NULL;
    if( !g_utf8_validate(text, len, NULL) ) {
        utf8_text = g_convert(text,len,"UTF-8","ISO-8859-1",NULL,NULL,NULL);
    }
    return
        utf8_text ? utf8_text :
        len>=0    ? g_strndup(text,len) : g_strdup(text);
}

static GtkWidget *
get_widget(GtkBuilder *builder, const gchar *widget_name)
{
    return GTK_WIDGET( gtk_builder_get_object( builder, widget_name ) );
}

/**
 * set_entry - Sets the text of a GtkEntry widget.
 * @builder:     a GtkBuilder instance
 * @widget_name: the name of the GtkEntry widget to be set
 * @text:        the text to be set in the GtkEntry widget
 * @len: the length of the text, or -1 if the text is NUL-terminated
 *
 * Sets the text of a GtkEntry widget with the given name in the specified
 * GtkBuilder instance. If @len is -1, the text is assumed to be
 * NUL-terminated. The text is expected to be in valid UTF-8 format.
 */ 
static void
set_entry(GtkBuilder  *builder,
          const gchar *widget_name,
          const gchar *text,
          int          len)
{
    GtkEntry *entry; GtkEntryBuffer *buffer;
    gchar *utf8_text = ensure_valid_utf8( text, len );

    entry = GTK_ENTRY( get_widget( builder, widget_name ) );
    if (entry) {
        buffer = gtk_entry_get_buffer( entry );
        gtk_entry_buffer_set_text( buffer, utf8_text, -1 );
    }
    g_free(utf8_text);
}

/**
 * set_text_view - Sets the text of a GtkTextView widget.
 * @gtk_builder:     a GtkBuilder instance
 * @gtk_widget_name: the name of the GtkTextView widget to be set
 * @text:            the text to be set in the GtkTextView widget
 * @len: the length of the text, or -1 if the text is NUL-terminated
 *
 * Sets the text of a GtkTextView widget with the given name in the specified
 * GtkBuilder instance. If @len is -1, the text is assumed to be
 * NUL-terminated. The text is expected to be in valid UTF-8 format.
 */ 
static void
set_text_view(GtkBuilder  *gtk_builder,
              const gchar *gtk_widget_name,
              const gchar *text,
              int          len)
{    
    GtkTextView *text_view; GtkTextBuffer *buffer;
    gchar *utf8_text = ensure_valid_utf8( text, len );
    
    text_view = GTK_TEXT_VIEW(
        gtk_builder_get_object( gtk_builder, gtk_widget_name )
    );
    if (text_view) {
        buffer = gtk_text_view_get_buffer( text_view );
        gtk_text_buffer_set_text( buffer, utf8_text, -1 );
    }
    g_free(utf8_text);
}

