
#include <string.h>

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
parsed_param_trim(const char **inout_text, int *inout_text_size) {
    const char *ptr; int size;
    ptr = (*inout_text); size = (*inout_text_size);
    while (size>0 && *ptr<=' ')        { size--; ptr++; }
    while (size>0 && ptr[size-1]<=' ') { size--; }
    (*inout_text) = ptr; (*inout_text_size) = size;
}

static void
parsed_param_key(const char **out_key,
                 int         *out_key_size,
                 const char  *param,
                 int          param_size)
{
    const char *ptr; int size, size_limit;
    ptr = param; size = 0; size_limit = param_size;
    while( size<size_limit && ptr[size]!=KEYVALUE_SEPARATOR ) { ++size; }
    parsed_param_trim( &ptr, &size );
    (*out_key) = ptr; (*out_key_size) = size;
}

static void
parsed_param_value(const char **out_value,
                   int         *out_value_size,
                   const char  *param,
                   int          param_size)
{
    const char *ptr; int size;
    ptr = param; size = param_size;
    while( size>0 && ptr[0]!=KEYVALUE_SEPARATOR ) { --size; ++ptr; }
    if   ( size>0 && ptr[0]==KEYVALUE_SEPARATOR ) { --size; ++ptr; }
    parsed_param_trim( &ptr, &size );
    (*out_value) = ptr; (*out_value_size) = size;
}

static const int
parse_next_param(const char **out_param,
                 int         *out_param_size,
                 const char **inout_buffer,
                 int         *inout_buffer_size)
{
/* This parser can handle parameters that come in one of three formats:
 * 1. (alphanumeric + spaces) ':' (spaces) (      chars not ','       ) ','
 * 2. (alphanumeric + spaces) ':' (spaces) '"' (any chars) '"' (spaces) ','
 * 3. (alphanumeric + spaces) ':' (spaces) '{' (any chars) '}' (spaces) ','
 */
    const char *ptr, *param; char close_char; int size, param_size=0;
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

static const char*
parse_params_find_last_line(const char *text, int text_size) {
    const char *ptr, *param, *lastline; int size, param_size;
    
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

static const char*
parse_params_find_negative(const char *text, int text_size) {
    const char *ptr, *charsleft, *negative_found; int size;
    
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
parse_sd_params_from_last_line(const char             *last_line,
                               ParseParametersCallback callback,
                               void                   *user_ptr,
                               int                     user_int)
{
    const char *ptr, *key, *value, *param;
    int size, key_size, value_size, param_size;
    
    ptr  = last_line;
    size = strlen(last_line);
    while( parse_next_param(&param, &param_size, &ptr, &size) ) {
        parsed_param_key  ( &key  , &key_size  , param, param_size );
        parsed_param_value( &value, &value_size, param, param_size );
        if( key_size>0 && value_size>0 ) {
            callback(key, key_size, value, value_size, user_ptr, user_int);
        }
    }
}

/* The parameter parsing logic used in this function is similar to that used
 * by the 'parse_generation_parameters' function in the AUTOMATIC1111 WebUI.
 * Although it does not follow the exact sequence of operations performed
 * there, the final result is equivalent.
 */
static void
parse_sd_parameters(const char              *text,
                    int                      text_size,
                    ParseParametersCallback  callback,
                    void                    *user_ptr,
                    int                      user_int)
{
    const char *prompt;   int prompt_size;
    const char *negative; int negative_size;
    const char *lastline;

    /* extract the 3 main lines */
    prompt      = text;
    prompt_size = text_size>=0 ? text_size : strlen(text);
    lastline    = parse_params_find_last_line(prompt, prompt_size);
    if( lastline ) {
        prompt_size = (lastline - prompt);
    }    
    negative      = parse_params_find_negative(prompt, prompt_size);
    negative_size = 0;
    if( negative ) {
        negative_size = prompt_size - (negative - prompt);
        prompt_size   = (negative - prompt);
        parsed_param_value( &negative, &negative_size, negative, negative_size );
    }
    
    /* send extracted information */
    if( prompt_size ) {
        callback("Prompt", 6, prompt, prompt_size, user_ptr, user_int);
    }
    if ( negative_size ) {
        callback("Negative prompt", 15, negative, negative_size, user_ptr, user_int);
    }
    if( lastline ) {
        parse_sd_params_from_last_line( lastline, callback, user_ptr, user_int );
    }
}
