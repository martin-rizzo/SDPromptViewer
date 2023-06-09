/**
 * @file    utils_sdparams.h
 * @brief   This file implement a SD parameters parser in bare C code.
 * @author  Martin Rizzo | <martinrizzo@gmail.com>
 * @date    Mar 29, 2023
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

    USAGE EXAMPLE
    -------------

      #include "utils_sdparams.h"
      
      int main()
      {
          FILE *file;
          SDParameters *sd_parameters;
          
          sd_parameters = calloc( 1, sizeof(SDParameters) );
          
          file = fopen("image_parameters.txt", "rb");
          if( !file ) { return 1; }
          fread( sd_parameters->input, SD_PARAMETERS_INPUT_SIZE, 1, file );
          fclose( file );
          
          parse_sd_parameters( sd_parameters );
          
          [  ... use the information stored in 'sd_parameters' ...  ]
          
          free( sd_parameters );
          return 0;
      }
    
*/
#include <stdlib.h>
#include <string.h>

/* Maximum size in bytes of the 'input' field in the SDParameters struct. */
#define SD_PARAMETERS_INPUT_SIZE (32*1024)

/* Maximum size of any array in the SDParameters struct. */
#define SD_PARAMETERS_ARRAY_SIZE 64

/**
 * Struct used to define input and output parameters for a task. It is used
 * by the parse_sd_parameters_from_buffer function to parse the input buffer
 * and populate output parameters with the recognized values.
 */
typedef struct _SDParameters SDParameters;
struct         _SDParameters {
    /* input buffer */
    char input[SD_PARAMETERS_INPUT_SIZE];
    int _zero_padding;
        
    /* output parameters */
    const char *prompt;
    const char *negative_prompt;
    const char *wildcard_prompt;
    const char *sampler;
    const char *steps;
    const char *cfg_scale;
    const char *seed;
    const char *width;
    const char *height;
    const char *denoising;
    
    struct { /* model */
        int         has_info;
        const char *name;
        const char *hash;
    } model;
    
    struct { /* hires. fix */
        int         has_info;
        const char *upscaler;
        const char *steps;
        const char *denoising;
        const char *upscale;
        const char *width;
        const char *height;
        float       calc_upscale;
        float       calc_width;
        float       calc_height;
    } hires;
    
    struct { /* inpaint */
        int         has_info;
        const char *denoising;
        const char *mask_blur;
    } inpaint;
    
    struct { /* override settings */
        int         has_info;
        const char *eta;
        const char *ensd;
        const char *clip_skip;
    } settings;
    
    struct { /* unknown parameters [] */
        const char *key;
        const char *value;
    } unknowns[SD_PARAMETERS_ARRAY_SIZE];
    int unknowns_count;
};

typedef void (*SDParametersCallback)(SDParameters *sd_parameters,
                                     void         *user_ptr,
                                     int           user_int);


#define IS_ALPHA_SPACE(x) parse_sd_params_is_alpha_space(x)
#define IS_SPACE(x)       ((x)==' ' || (x)=='\t')

static int
parse_sd_params_is_alpha_space(int ch) {
    return ('a'<=ch && ch<='z') || ('A'<=ch && ch<='Z') ||
           ('0'<=ch && ch<='9') || (ch==' ');
}
    
typedef void (*ParseParametersCallback)(const char *key,
                                        int         key_size,
                                        const char *value,
                                        int         value_size,
                                        void       *user_ptr,
                                        int         user_int);



#define KEYVALUE_SEPARATOR ':'

static void
parsed_param_trim(char **inout_text, int *inout_text_size) {
    char *ptr; int size;
    ptr = (*inout_text); size = (*inout_text_size);
    while (size>0 && *ptr<=' ')        { size--; ptr++; }
    while (size>0 && ptr[size-1]<=' ') { size--; }
    (*inout_text) = ptr; (*inout_text_size) = size;
}

static void
parsed_param_key(char **out_key,
                 int   *out_key_size,
                 char  *param,
                 int    param_size)
{
    char *ptr; int size, size_limit;
    ptr = param; size = 0; size_limit = param_size;
    while( size<size_limit && ptr[size]!=KEYVALUE_SEPARATOR ) { ++size; }
    parsed_param_trim( &ptr, &size );
    (*out_key) = ptr; (*out_key_size) = size;
}

static void
parsed_param_value(char **out_value,
                   int   *out_value_size,
                   char  *param,
                   int    param_size)
{
    char *ptr; int size;
    ptr = param; size = param_size;
    while( size>0 && ptr[0]!=KEYVALUE_SEPARATOR ) { --size; ++ptr; }
    if   ( size>0 && ptr[0]==KEYVALUE_SEPARATOR ) { --size; ++ptr; }
    parsed_param_trim( &ptr, &size );
    (*out_value) = ptr; (*out_value_size) = size;
}

static const int
parse_next_param(char **out_param,
                 int   *out_param_size,
                 char **inout_buffer,
                 int   *inout_buffer_size)
{
/* This parser can handle parameters that come in one of three formats:
 * 1. (alphanumeric + spaces) ':' (spaces) (      chars not ','       ) ','
 * 2. (alphanumeric + spaces) ':' (spaces) '"' (any chars) '"' (spaces) ','
 * 3. (alphanumeric + spaces) ':' (spaces) '{' (any chars) '}' (spaces) ','
 */
    char *ptr, *param; char close_char; int size, param_size=0;
    ptr   = (*inout_buffer);
    size  = (*inout_buffer_size);
    param = ptr;
    
    /* skip alphanumeric+spaces ':' spaces */
    while( size>0 && IS_ALPHA_SPACE(*ptr) ) { --size; ++ptr; }
    if( size==0 || *ptr!=':' ) { return 0; }
    --size; ++ptr;
    while( size>0 && IS_SPACE(*ptr) ) { --size; ++ptr; }
    
    /* If the buffer runs out (size == 0) or the end of a */
    /* line is reached (*ptr == '\n'), an error occurs    */
    if( size==0 || *ptr=='\n' ) { return 0; }
    
    /* at this point in the code, '*ptr' indicates the opening character */
    /* then let's determine closing character based on it                */
    switch( *ptr )
    {
        case '"': close_char = '"'; break;
        case '{': close_char = '}'; break;
        default : close_char = ','; break;
    }
    --size; ++ptr;
    while( size>0 && *ptr!=close_char ) { --size; ++ptr; }
    if( size==0 && close_char!=',' ) { return 0; }
    while( size>0 && *ptr!=',' ) { --size; ++ptr; }
    
    /* at this point (*ptr) is equal to ',' or the end of the buffer */
    param_size = (ptr - param);
    if( size>0 ) { --size; ++ptr; }
    (*out_param        ) = param;
    (*out_param_size   ) = param_size;
    (*inout_buffer     ) = ptr;
    (*inout_buffer_size) = size;
    return 1;
}

static char*
parse_params_find_last_line( char *text, int text_size ) {
    char *ptr, *param, *lastline; int size, param_size;
    
    if( text_size==0 ) { return NULL; }
    if( text_size==1 && *text=='\n' ) { return NULL; }
    
    /* find the beginning of the last line */
    ptr = &text[text_size-2]; size = 2;
    while( size<text_size && *ptr!='\n' ) { ++size; --ptr; }
    if( *ptr!='\n' ) { return NULL; }
    --size; ++ptr;
    
    /* verify the last line contains at least 2 params */
    lastline = ptr;
    if( !parse_next_param(&param, &param_size, &ptr, &size) ) { return NULL; }
    if( !parse_next_param(&param, &param_size, &ptr, &size) ) { return NULL; }
    return lastline;
}

static char*
parse_params_find_negative( char *text, int text_size ) {
    char *ptr, *charsleft, *negative_found; int size;
    
    negative_found = NULL; ptr = text; size = text_size; 
    while( !negative_found && size>0  )
    {
        negative_found = ptr;
        charsleft = "Negative prompt:";
        while( size>0 && *charsleft && *ptr==*charsleft ) {
            --size; ++ptr; ++charsleft;
        }
        if( *charsleft ) {
            negative_found = NULL;
            while( size>0 && *ptr!='\n' ) { --size; ++ptr; }
            while( size>0 && *ptr=='\n' ) { --size; ++ptr; }
        }
    }
    return negative_found;
}

static void
parse_sd_params_set_wxh(SDParameters *sd_parameters,
                        char         *str_value,
                        int           hires)
{
    char *ptr, *str_width, *str_height; int size;
    ptr  = str_value;
    size = strlen( str_value );
    
    str_width = ptr;
    while( size>0 && '0'<=*ptr && *ptr<='9' ) { --size; ++ptr; }
    *ptr = '\0';
    if( size>0 ) { --size; ++ptr; }
    str_height = ptr;
    
    if( hires ) {
        sd_parameters->hires.width  = str_width;
        sd_parameters->hires.height = str_height;
    } else {
        sd_parameters->width  = str_width;
        sd_parameters->height = str_height;
    }
}

static void
parse_sd_params_set(SDParameters *sd_parameters,
                    char         *str_key,
                    char         *str_value)
{
#   define IF_KEY(keyname)   if( 0==strcmp(keyname,str_key))
#   define ELIF_KEY(keyname) else IF_KEY(keyname)
    
      IF_KEY("Prompt"          ) sd_parameters->prompt            = str_value;   
    ELIF_KEY("Negative prompt" ) sd_parameters->negative_prompt   = str_value; 
    ELIF_KEY("Wildcard prompt" ) sd_parameters->wildcard_prompt   = str_value; 
    ELIF_KEY("Model"           ) sd_parameters->model.name        = str_value; 
    ELIF_KEY("Model hash"      ) sd_parameters->model.hash        = str_value; 
    ELIF_KEY("Sampler"         ) sd_parameters->sampler           = str_value;  
    ELIF_KEY("Steps"           ) sd_parameters->steps             = str_value;        
    ELIF_KEY("CFG scale"       ) sd_parameters->cfg_scale         = str_value; 
    ELIF_KEY("Seed"            ) sd_parameters->seed              = str_value; 
    ELIF_KEY("Denoising strength") sd_parameters->denoising       = str_value; 
    ELIF_KEY("Hires upscaler"  ) sd_parameters->hires.upscaler    = str_value; 
    ELIF_KEY("Hires steps"     ) sd_parameters->hires.steps       = str_value;
    ELIF_KEY("Hires upscale"   ) sd_parameters->hires.upscale     = str_value;
    ELIF_KEY("Mask blur"       ) sd_parameters->inpaint.mask_blur = str_value;
    ELIF_KEY("Eta"             ) sd_parameters->settings.eta      = str_value;
    ELIF_KEY("ENSD"            ) sd_parameters->settings.ensd     = str_value;
    ELIF_KEY("Clip skip"       ) sd_parameters->settings.clip_skip= str_value;
    ELIF_KEY("Size") {
        parse_sd_params_set_wxh( sd_parameters, str_value, 0 ); }
    ELIF_KEY("Hires resize") {
        parse_sd_params_set_wxh( sd_parameters, str_value, 1 ); }
    else {
        const int index = sd_parameters->unknowns_count++;
        if( index < (SD_PARAMETERS_ARRAY_SIZE-1) ) {
            sd_parameters->unknowns[index].key   = str_key;
            sd_parameters->unknowns[index].value = str_value;
        }
    }
#   undef IF_KEY
#   undef ELIF_KEY
}

static void
parse_sd_params_from_lastline(SDParameters *sd_parameters,
                              char         *lastline,
                              int           lastline_size)
{
    char *ptr, *key, *value, *param;
    int size, key_size, value_size, param_size;
    
    ptr  = lastline;
    size = lastline_size;
    while( parse_next_param(&param, &param_size, &ptr, &size) ) {
        parsed_param_key  ( &key  , &key_size  , param, param_size );
        parsed_param_value( &value, &value_size, param, param_size );
        if( key_size>0 && value_size>0 ) {
            key  [ key_size   ] = '\0';
            value[ value_size ] = '\0';
            parse_sd_params_set( sd_parameters, key, value );
        }
    }
}

static void
parse_sd_params_final_fix(SDParameters *sd_parameters)
{
    const char* denoising; char *endptr;
    float width, height, hr_width, hr_height, hr_upscale; int n;
    
    /* 1) set 'has_info' field in each sub-group */
    sd_parameters->model.has_info = 
      ( sd_parameters->model.name != NULL ) ||
      ( sd_parameters->model.hash != NULL );

    sd_parameters->hires.has_info =
      ( sd_parameters->hires.upscaler != NULL ) ||
      ( sd_parameters->hires.steps    != NULL ) ||
      ( sd_parameters->hires.upscale  != NULL ) ||
      ( sd_parameters->hires.width    != NULL ) ||
      ( sd_parameters->hires.height   != NULL );
      
    sd_parameters->inpaint.has_info = 
      ( sd_parameters->inpaint.mask_blur != NULL );
      
    sd_parameters->settings.has_info =
      ( sd_parameters->settings.eta       != NULL ) ||
      ( sd_parameters->settings.ensd      != NULL ) ||
      ( sd_parameters->settings.clip_skip != NULL );
    
    /* 2) identify denoising sub-group */
    denoising = sd_parameters->denoising;
    if( sd_parameters->inpaint.has_info ) {
        if( !sd_parameters->inpaint.denoising ) {
             sd_parameters->inpaint.denoising = denoising;
        }
    } else if( sd_parameters->hires.has_info ) {
        if( !sd_parameters->hires.denoising ) {
             sd_parameters->hires.denoising = denoising;
        }
    }
    
    /* 3) calculate hires width, height & upscale fields */
    width = sd_parameters->width ?
        (float)strtod( sd_parameters->width, &endptr ) : 0;
    height = sd_parameters->height ?
        (float)strtod( sd_parameters->height, &endptr ): 0;
    hr_width = sd_parameters->hires.width ?
        (float)strtod( sd_parameters->hires.width, &endptr ) : 0;
    hr_height = sd_parameters->hires.height ?
        (float)strtod( sd_parameters->hires.height, &endptr ) : 0;
    hr_upscale = sd_parameters->hires.upscale ?
        (float)strtod( sd_parameters->hires.upscale,  &endptr ) : 0;
        
    if( hr_width  == 0.0f ) {
        sd_parameters->hires.calc_width = width  * hr_upscale;
    }
    if( hr_height == 0.0f ) {
        sd_parameters->hires.calc_height = height * hr_upscale;
    }
    if( hr_upscale == 0.0f ) {
        n = 0;
        if( hr_width>0  && width>0  ) { hr_upscale += hr_width/width;   n++; }
        if( hr_height>0 && height>0 ) { hr_upscale += hr_height/height; n++; }
        if( n>0 ) {
            sd_parameters->hires.calc_upscale = hr_upscale / (float)n;
        }
    }
}

/**
 * Parses SD generation parameters from the 'input' field and populates
 * the corresponding output fields with them.
 * 
 * The 'input' field of the SDParameters struct is expected to contain a
 * text string with the prompt and all the parameters used when generating
 * an image with Stable Diffusion.
 * 
 * The function parses each parameter it finds and stores it in the
 * corresponding field of the SDParameters struct. 
 * 
 * The function parses each parameter it finds and stores it in the
 * corresponding field of the SDParameters struct. Parameters that cannot
 * be identified are stored in the 'unknowns' array of the struct.
 * 
 * The parameter parsing logic used in this function is similar to that used
 * by the 'parse_generation_parameters' function in the AUTOMATIC1111 WebUI.
 * Although it does not follow the exact sequence of operations performed
 * there, the final result is equivalent.
 * 
 * @param sd_parameters A pointer to the SDParameters struct that contains
 *    the input text in the 'input' field, and will be populated with the
 *    identified generation parameters.
 */
static void
parse_sd_parameters(SDParameters *sd_parameters)
{
    char *prompt;   int prompt_size   = 0;
    char *negative; int negative_size = 0;
    char *lastline; int lastline_size = 0;

    /* extract the 3 main lines */
    prompt      = sd_parameters->input;
    prompt_size = strlen( prompt );
    lastline    = parse_params_find_last_line( prompt, prompt_size );
    
    if( lastline ) {
        lastline_size = prompt_size - (lastline - prompt);
        prompt_size   = (lastline - prompt) - 1;
    }    
    negative      = parse_params_find_negative( prompt, prompt_size );
    negative_size = 0;
    if( negative ) {
        negative_size = prompt_size - (negative - prompt);
        prompt_size   = (negative - prompt) - 1;
        parsed_param_value( &negative, &negative_size, negative, negative_size );
    }
    
    /* store extracted information */
    if( prompt_size ) {
        prompt[ prompt_size ] = '\0';
        sd_parameters->prompt = prompt;
    }
    if ( negative_size ) {
        negative[ negative_size ] = '\0';
        sd_parameters->negative_prompt = negative;
    }
    if( lastline ) {
        parse_sd_params_from_lastline(sd_parameters, lastline, lastline_size);
    }   
    
    /* final calculation of the remaining fields */
    parse_sd_params_final_fix( sd_parameters );
}

/**
 * Parses SD generation parameters from the text buffer and populates
 * the corresponding output fields with them.
 * 
 * This function is similar to 'parse_sd_parameters', but it takes a
 * buffer containing the input text. The buffer is copied to the 'input'
 * field of the struct to facilitate the parameter parsing task.
 * 
 * The function parses each parameter it finds in the input buffer and
 * stores it in the corresponding field of the SDParameters struct.
 * Parameters that cannot be identified are stored in the 'unknowns'
 * array of the struct.
 * 
 * The parameter parsing logic used in this function is similar to that used
 * by the 'parse_generation_parameters' function in the AUTOMATIC1111 WebUI.
 * Although it does not follow the exact sequence of operations performed
 * there, the final result is equivalent.
 * 
 * @param sd_parameters A pointer to the SDParameters struct that will be
 *    populated with the identified generation parameters.
 * @param buffer A pointer to the buffer (or string) containing the input
 *    text to be parsed.
 * @param buffer_size The number of bytes in the buffer or -1 if buffer
 *    contains a null-terminated string.
 */
static void
parse_sd_parameters_from_buffer(SDParameters *sd_parameters,
                                const char   *buffer,
                                int           buffer_size)
{    
    /* copy buffer */
    if( buffer_size < 0 ) { buffer_size = strlen( buffer ); }
    if( buffer_size > SD_PARAMETERS_INPUT_SIZE ) {
        buffer_size = SD_PARAMETERS_INPUT_SIZE;
    }
    memset( sd_parameters, 0, sizeof(SDParameters) );
    memcpy( sd_parameters->input, buffer, buffer_size);
    parse_sd_parameters( sd_parameters );
}


