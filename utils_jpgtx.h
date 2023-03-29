/**
 * @file    utils_jpgtx.h
 * @date    Mar 27, 2023
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
     
     
 TODO:
   - asynchronous implementation
   
 */

typedef void (*JPGTextCallback)(gchar   *text,
                                gpointer data_ptr,
                                int      data_int);

typedef struct _JPGTextMessage JPGTextMessage;
struct         _JPGTextMessage {
    JPGTextCallback  callback;
    gpointer         data_ptr;
    int              data_int;
};


/*
static const gchar *
get_file_path_with_new_extension(GFile *file, const gchar *extension) {
    gchar *new_path, *path, *dot_post;
    path    = g_file_get_path(file);
    dot_pos = strrchr(path, '.');
    if( dot_pos != NULL ) { *dot_pos = '\0'; }
    new_path = g_strdup_printf("%s%s", path, extension);
    g_free(path);
    return new_path;
}

static void
load_jpg_text(GFile           *file,
              JPGTextCallback  callback,
              gpointer         data_ptr,
              int              data_int)
{
    
    JPGTextMessage *message = g_new(JPGTextMessage, 1);
    message->callback = callback;
    message->data_ptr = data_ptr;
    message->data_int = data_int;
    g_free(message);

    char* file_path = g_file_get_path(file);
    g_free(file_path);

}
*/
