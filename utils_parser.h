
#include <string.h>

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
parsed_param_key(const char **inout_text, int *inout_text_size) {
    const char *ptr; int size, size_limit;
    ptr = (*inout_text); size = 0; size_limit = (*inout_text_size);
    while( size<size_limit && ptr[size]!=KEYVALUE_SEPARATOR ) { ++size; }
    parsed_param_trim( &ptr, &size );
    (*inout_text) = ptr; (*inout_text_size) = size;
}

static void
parsed_param_value(const char **inout_text, int *inout_text_size) {
    const char *ptr; int size;
    ptr = (*inout_text); size = (*inout_text_size);
    while( size>0 && ptr[0]!=KEYVALUE_SEPARATOR ) { --size; ++ptr; }
    if   ( size>0 && ptr[0]==KEYVALUE_SEPARATOR ) { --size; ++ptr; }
    parsed_param_trim( &ptr, &size );
    (*inout_text) = ptr; (*inout_text_size) = size;
}

static char*
parse_params_find_last_line(const char *text, int text_size) {
    return NULL;
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
        parsed_param_value( &negative, &negative_size );
    }
    
    /* send extracted information */
    if( prompt_size ) {
        callback("Prompt", 6, prompt, prompt_size, user_ptr, user_int);
    }
    if ( negative_size ) {
        callback("Negative prompt", 15, negative, negative_size, user_ptr, user_int);
    }
    /*
    if( lastline ) {
        parse_prompt_last_line( lastline, callback, user_ptr, user_int );
    }
    */
}
