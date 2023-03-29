/**
 * @file    utils_png.h
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
     
 
 TODO:
   - asynchronous implementation
   
 */
#include <glib.h>

typedef void (*PNGTextChunkCallback)(gchar   *text,
                                     gpointer data_ptr,
                                     int      data_int);

typedef struct _PNGTextChunkMessage PNGTextChunkMessage;
struct         _PNGTextChunkMessage {
    GFile                *file;
    gchar                *key;
    PNGTextChunkCallback  callback;
    gpointer              data_ptr;
    int                   data_int;
};


static const guint8 PNG_SIGNATURE[] = {137, 80, 78, 71, 13, 10, 26, 10};
#define PNG_SIGNATURE_LENGTH sizeof(PNG_SIGNATURE)
#define CHUNK_HEADER_SIZE 8 /* CHUNK_LENGTH + CHUNK_TYPE */
#define CHUNK_CRC_SIZE    4

/*-------------------------- READ/SKIP BYTES ----------------------------*/

static gboolean
read_png_bytes(GInputStream *input_stream, void *buffer, gsize count)
{
    gsize bytes_read;
    return
    g_input_stream_read_all(input_stream, buffer, count, &bytes_read,
                            NULL, NULL)
    ? (bytes_read==count) : FALSE;
}

static gboolean
skip_png_bytes(GInputStream *input_stream, gsize count)
{
    return g_input_stream_skip(input_stream, count, NULL, NULL) == count;
}

static gboolean
has_png_signature(GInputStream *input_stream)
{
    guint8 signature_buffer[PNG_SIGNATURE_LENGTH];
    if( !read_png_bytes(input_stream, signature_buffer, PNG_SIGNATURE_LENGTH) ) {
        return FALSE;
    }
    return memcmp(signature_buffer, PNG_SIGNATURE, PNG_SIGNATURE_LENGTH)==0;    
}


/*---------------------------- PROCESS CHUNKS -----------------------------*/

static PNGTextChunkMessage *
load_png_text_chunk_completed(gchar *text, PNGTextChunkMessage *message);
#define DISPATCH(value,message) load_png_text_chunk_completed(value,message)
#define DISPATCH_ERROR(message) load_png_text_chunk_completed("",message)


/* Funcion invocada cada vez que se carga un chunk de texto de algun PNG */
static PNGTextChunkMessage *
process_png_text_chunk(GInputStream        *input_stream,
                       gsize                chunk_size,
                       PNGTextChunkMessage *message)
{
    int i; gchar *chunk_data, *key=NULL, *value=NULL;
    
    chunk_data = g_new(char, chunk_size+1);
    chunk_data[chunk_size] = '\0';
    
    if( message ) {
        if( !read_png_bytes(input_stream, chunk_data, chunk_size) ) {
            message = DISPATCH_ERROR(message);
        }
    }
    if( message ) {
        key = chunk_data;
        for( i=0 ; (!value) && i<(chunk_size-1) ; ++i ) {
            if( chunk_data[i] == '\0' ) { value = &chunk_data[i+1]; }
        }
    }
    if( message ) {
        if( key!=NULL && value!=NULL && g_strcmp0(key,message->key)==0 ) {
            message = DISPATCH(value,message);
        }
    }
    g_free(chunk_data);
    return message;
}

static PNGTextChunkMessage *
process_png_chunk(GInputStream        *input_stream,
                  PNGTextChunkMessage *message) {
    
    gsize  chunk_size;
    char  *chunk_type;
    guint8 chunk_header[CHUNK_HEADER_SIZE+1];
    chunk_header[CHUNK_HEADER_SIZE] = '\0';
    
    if( message ) {
        if ( !read_png_bytes(input_stream, chunk_header, CHUNK_HEADER_SIZE) ) {
            message = DISPATCH_ERROR(message);
        }
    }
    if( message ) {
        chunk_size = (chunk_header[0] << 24) |
                     (chunk_header[1] << 16) |
                     (chunk_header[2] <<  8) |
                     (chunk_header[3]      );
        chunk_type = (char *)( &chunk_header[ 4 ] );
        if( g_strcmp0(chunk_type,"tEXt") == 0 ) {
            message = process_png_text_chunk(input_stream, chunk_size, message);
        }
        else if( !skip_png_bytes(input_stream, chunk_size+CHUNK_CRC_SIZE) ) {
            message = DISPATCH_ERROR(message);
        }
    }
    return message;
}

static void
process_text_chunk_message(PNGTextChunkMessage* message)
{
    GFileInputStream *input_stream = NULL;

    if( message ) {
        input_stream = g_file_read(message->file, NULL, NULL);
        if( !input_stream ) { message = DISPATCH_ERROR(message); }
    }
    if( message ) {
        if( !has_png_signature( G_INPUT_STREAM(input_stream) ) ) {
            message = DISPATCH_ERROR(message);
        }
    }
    while( message ) {
        message = process_png_chunk( G_INPUT_STREAM(input_stream), message );
    }
    /* el mensaje debe ser enviado si o si */
    if( message ) {
        message = DISPATCH_ERROR(message);
    }
    /* cleanup */
    if( input_stream ) {
        g_input_stream_close(G_INPUT_STREAM(input_stream), NULL, NULL);
        g_object_unref(input_stream);
    }
}

/*============================ MAIN FUNCTION ==============================*/


static void
load_png_text_chunk(GFile               *file,
                    gchar               *key,
                    PNGTextChunkCallback callback,
                    gpointer             data_ptr,
                    int                  data_int)
{
    PNGTextChunkMessage *message = g_new(PNGTextChunkMessage, 1);
    message->file     = g_object_ref(file);
    message->key      = g_strdup(key);
    message->callback = callback;
    message->data_ptr = data_ptr;
    message->data_int = data_int;
    process_text_chunk_message(message);
}

static PNGTextChunkMessage *
load_png_text_chunk_completed(gchar *text, PNGTextChunkMessage *message)
{
    message->callback( text, message->data_ptr, message->data_int );
    g_object_unref(message->file);
    g_free        (message->key);
    g_free(message);
    return NULL;
}



