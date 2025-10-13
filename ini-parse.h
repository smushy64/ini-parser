#if !defined(INI_PARSE_H)
#define INI_PARSE_H
/**
 * @file   ini-parse.h
 * @brief  Header-only simple .ini parser.
 * @author Alicia Amarilla (smushyaa@gmail.com)
 * @date   October 10, 2025
*/
// IWYU pragma: begin_keep
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
// IWYU pragma: end_keep

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#if defined(__cplusplus)
    #define INI_INL inline
#else
    #define INI_INL static inline
#endif

struct IniReadSettings {
    const char* default_value;
    const char* comment;
    bool*       is_found;
    bool*       is_type_correct;
    bool        dont_create;
};

enum IniType {
    INI_TYPE_NULL,
    INI_TYPE_STRING,
    INI_TYPE_NUMBER,
    INI_TYPE_BOOLEAN,

    INI_TYPE_COUNT
};

struct IniStr {
    size_t len;
    size_t off;
};

struct IniKV {
    struct IniStr comment;

    struct IniStr  key;
    struct IniStr  value;
    enum   IniType type;

    union {
        double number;
        bool   boolean;
    };
};

struct IniSection {
    struct IniStr comment;

    struct IniStr name;

    size_t        cap;
    size_t        len;
    struct IniKV* ptr;
};

typedef struct {
    struct {
        size_t cap;
        size_t len;
        char*  ptr;
    } string;

    size_t             cap;
    size_t             len;
    struct IniSection* ptr;
} IniCtx;

typedef size_t IniSerializeFN( void* target, const char* fmt, va_list va );

INI_INL
bool ini_open( const char* path, IniCtx* out_ctx );

INI_INL
bool ini_open_from_memory( size_t memory_size, const void* memory, IniCtx* out_ctx );

INI_INL
void ini_new( IniCtx* ctx );

INI_INL
void ini_close( IniCtx* ctx );

INI_INL
size_t ini_serialize( IniCtx* ctx, void* target, IniSerializeFN* fn );

INI_INL
bool ini_serialize_to_file( IniCtx* ctx, const char* path, bool should_truncate );

#define ini_read( ctx, section, key, ... ) \
    __ini_read( ctx, section, key, (struct IniReadSettings){ __VA_ARGS__ } )

#define ini_read_number( ctx, section, key, ... ) \
    __ini_read_number( ctx, section, key, (struct IniReadSettings){ __VA_ARGS__ } )

#define ini_read_bool( ctx, section, key, ... ) \
    __ini_read_bool( ctx, section, key, (struct IniReadSettings){ __VA_ARGS__ } )

INI_INL
void ini_write_va(
    IniCtx* ctx, const char* section, const char* key, const char* value, va_list va );

INI_INL
void ini_write(
    IniCtx* ctx, const char* section, const char* key, const char* value, ... );

INI_INL
void ini_set_section_comment_va( IniCtx* ctx, const char* section, const char* comment, va_list va );

INI_INL
void ini_set_section_comment( IniCtx* ctx, const char* section, const char* comment, ... );

INI_INL
void ini_set_comment_va(
    IniCtx* ctx, const char* section, const char* key, const char* comment, va_list va );

INI_INL
void ini_set_comment(
    IniCtx* ctx, const char* section, const char* key, const char* comment, ... );

INI_INL
const char* string_from_ini_type( enum IniType type );

// NOTE(alicia): internal functions -------------------------------------------

INI_INL
const char* __ini_read(
    IniCtx* ctx, const char* section, const char* key, struct IniReadSettings settings );
INI_INL
double __ini_read_number(
    IniCtx* ctx, const char* section, const char* key, struct IniReadSettings settings );
INI_INL
bool __ini_read_bool(
    IniCtx* ctx, const char* section, const char* key, struct IniReadSettings settings );

// NOTE(alicia): implementation -----------------------------------------------

#define RESERVE( Buf, Minimum ) do {                               \
    size_t __remaining__ = (Buf)->cap - (Buf)->len;                \
    if( __remaining__ < (Minimum) ) {                              \
        size_t __new_cap__  = (Buf)->cap + (Minimum) + 16;         \
        size_t __old_size__ = sizeof((Buf)->ptr[0]) * (Buf)->cap;  \
        size_t __new_size__ = sizeof((Buf)->ptr[0]) * __new_cap__; \
        (Buf)->ptr = (__typeof((Buf)->ptr))                        \
            realloc( (Buf)->ptr, __new_size__ );                   \
        (Buf)->cap = __new_cap__;                                  \
        if( __old_size__ ) {                                       \
            memset(                                                \
                (char*)((Buf)->ptr) + __old_size__,                \
                0, __new_size__ - __old_size__ );                  \
        }                                                          \
    }                                                              \
} while(0)

#define PUSH( Buf, Item ) do {         \
    RESERVE( Buf, 1 );                 \
    (Buf)->ptr[(Buf)->len++] = (Item); \
} while(0)

#define APPEND( Buf, Count, Items ) do {            \
    RESERVE( Buf, Count );                          \
    memcpy(                                         \
        (Buf)->ptr + (Buf)->len,                    \
        (Items), sizeof((Buf)->ptr[0]) * (Count) ); \
    (Buf)->len += (Count);                          \
} while(0)

INI_INL
struct IniStr __ini_str_va( IniCtx* ctx, const char* fmt, va_list va ) {
    struct IniStr result;

    va_list va2;
    va_copy( va2, va );

    result.len = vsnprintf( NULL, 0, fmt, va2 );

    va_end( va2 );

    RESERVE( &ctx->string, result.len + 16 );

    vsnprintf( ctx->string.ptr + ctx->string.len, ctx->string.cap - ctx->string.len, fmt, va );

    result.off = ctx->string.len;
    ctx->string.len += result.len + 1;

    return result;
}
INI_INL
struct IniStr __ini_str( IniCtx* ctx, const char* fmt, ... ) {
    va_list va;
    va_start( va, fmt );

    struct IniStr result = __ini_str_va( ctx, fmt, va );

    va_end( va );
    return result;
}

INI_INL
void ini_new( IniCtx* ctx ) {
    ini_open_from_memory( 0, NULL, ctx );
}

INI_INL
bool ini_open( const char* path, IniCtx* ctx ) {
    struct {
        size_t cap;
        size_t len;
        char*  ptr;
    } contents;
    memset( &contents, 0, sizeof(contents) );

    FILE* file = fopen( path, "rb" );

    if( !file ) {
        return ini_open_from_memory( 0, NULL, ctx );
    }

    fseeko( file, 0, SEEK_END );
    size_t size = ftello( file );
    fseeko( file, 0, SEEK_SET );

    RESERVE( &contents, size );

    fread( contents.ptr, size, 1, file );
    contents.len += size;

    fclose( file );

    bool result = ini_open_from_memory( contents.len, contents.ptr, ctx );

    free( contents.ptr );
    return result;
}

INI_INL
size_t __ini_section( IniCtx* ctx, size_t section_name_len, const char* section_name ) {
    bool   is_found = false;
    size_t index    = 0;

    bool is_null = false;
    if( section_name ) {
        if( section_name_len == 1 ) {
            is_null = section_name[0] == '*';
        }
    } else {
        is_null = true;
    }

    if( is_null ) {
        for( size_t i = 0; i < ctx->len; ++i ) {
            struct IniSection* section = ctx->ptr + i;

            if( section->name.len ) {
                continue;
            }

            is_found = true;
            index    = i;
            break;
        }
    } else {
        for( size_t i = 0; i < ctx->len; ++i ) {
            struct IniSection* section = ctx->ptr + i;

            if( section_name_len != section->name.len ) {
                continue;
            }

            if( memcmp(
                ctx->string.ptr + section->name.off,
                section_name,
                section_name_len
            ) ) {
                continue;
            }

            is_found = true;
            index    = i;
            break;
        }
    }

    if( is_found ) {
        return index;
    }

    index = ctx->len;

    struct IniSection section = {};
    if( !is_null ) {
        section.name = __ini_str( ctx, "%.*s", (int)section_name_len, section_name );
    }

    PUSH( ctx, section );

    return index;
}

INI_INL
void __ini_assign_value(
    IniCtx* ctx,
    struct IniKV* kv,
    size_t value_len, char* value,
    bool value_is_allocated
) {
    if( !value_len || !value ) {
        kv->type  = INI_TYPE_NULL;
        // value cannot already be allocated if it's null
        kv->value = __ini_str( ctx, "" );

        return;
    }

    if( value[0] == '"' ) {
        if( value_len && value[value_len - 1] == '"' ) {
            value++;
            value_len--;

            if( value_len && value[value_len - 1] == '"' ) {
                value_len--;
            }
        }

        if( value_len ) {
            kv->type  = INI_TYPE_STRING;
            if( value_is_allocated ) {
                kv->value.len = value_len;
                kv->value.off = (size_t)( value - ctx->string.ptr );
            } else {
                kv->value = __ini_str( ctx, "%.*s", (int)value_len, value );
            }

            return;
        } else {
            kv->type  = INI_TYPE_NULL;
            if( value_is_allocated ) {
                kv->value.len = value_len;
                kv->value.off = (size_t)( value - ctx->string.ptr );
            } else {
                kv->value = __ini_str( ctx, "" );
            }

            return;
        }
        return;
    }

    if( value_is_allocated ) {
        kv->value.len = value_len;
        kv->value.off = (size_t)( value - ctx->string.ptr );
    } else {
        kv->value = __ini_str( ctx, "%.*s", (int)value_len, value );
    }

    char* string = ctx->string.ptr + kv->value.off;

    char* endptr = NULL;
    double dbl = strtod( string, &endptr );

    if( (string == endptr) || ((string + kv->value.len) != endptr) ) {
        if( strcmp( string, "true" ) == 0 ) {
            kv->type    = INI_TYPE_BOOLEAN;
            kv->boolean = true;
        } else if( strcmp( string, "false" ) == 0 ) {
            kv->type    = INI_TYPE_BOOLEAN;
            kv->boolean = false;
        } else if( strcmp( string, "null" ) == 0 ) {
            kv->type  = INI_TYPE_NULL;
            kv->value = __ini_str( ctx, "" );
        } else {
            kv->type = INI_TYPE_STRING;
        }
    } else {
        kv->type   = INI_TYPE_NUMBER;
        kv->number = dbl;
    }
}

INI_INL
size_t __ini_field(
    IniCtx* ctx,
    size_t section_idx,
    size_t left_len, char* left,
    size_t right_len, char* right
) {
    struct IniSection* section = ctx->ptr + section_idx;

    bool   is_found = false;
    size_t index    = 0;
    for( size_t i = 0; i < section->len; ++i ) {
        struct IniKV* kv = section->ptr + i;

        if( left_len != kv->key.len ) {
            continue;
        }

        if( memcmp( left, ctx->string.ptr + kv->key.off, left_len ) ) {
            continue;
        }

        is_found = true;
        index    = i;
        break;
    }

    if( is_found ) {
        struct IniKV* kv = section->ptr + index;

        __ini_assign_value( ctx, kv, right_len, right, false );

        return index;
    }

    struct IniKV kv;
    memset( &kv, 0, sizeof(kv) );

    kv.key   = __ini_str( ctx, "%.*s", (int)left_len, left );
    __ini_assign_value( ctx, &kv, right_len, right, false );

    index = section->len;

    PUSH( section, kv );

    return index;
}

INI_INL
void __ini_trim_whitespace( size_t* len, char** ptr ) {
    size_t new_len = *len;
    char*  new_ptr = *ptr;

    for( size_t i = 0; i < new_len; ++i ) {
        if( !isspace( new_ptr[i] ) ) {
            new_len -= i;
            new_ptr += i;
            break;
        }
    }

    for( size_t i = new_len; i-- > 0; ) {
        if( !isspace( new_ptr[i] ) ) {
            if( (i + 1) < new_len ) {
                new_len = i + 1;
            }
            break;
        }
    }

    *len = new_len;
    *ptr = new_ptr;
}

INI_INL
bool ini_open_from_memory( size_t memory_size, const void* memory, IniCtx* ctx ) {
    memset( ctx, 0, sizeof(*ctx) );

    RESERVE( &ctx->string, 128 );
    ctx->string.len += 4;

    if( !memory || !memory_size ) {
        return true;
    }

    char* at = (char*)memory;

    size_t current_section = __ini_section( ctx, 0, 0 );

    size_t ln = 0;
    while( *at ) {
        char* nl = strchr( at, '\n' );
        ln++;

        if( !nl ) {
            break;
        }

        char*  line     = at;
        size_t line_len = at == nl ? 0 : nl - at;

        at = nl + 1;

        // skip empty lines
        if( !line_len ) {
            continue;
        }

        // skip all leading whitespace
        for( size_t i = 0; i < line_len; ++i ) {
            if( !isspace( line[i] ) ) {
                line     += i;
                line_len -= i;
                break;
            }
        }

        // skip whitespace only lines
        if( !line_len ) {
            continue;
        }

        // skip comment lines
        switch( line[0] ) {
            case ';':
            case '#':
                continue;
            default:
                break;
        }

        // could be a section
        if( line[0] == '[' ) {
            // section cannot be empty
            if( line_len < 3 ) {
                fprintf( stderr, "%4zu | section name is empty!\n", ln );
                continue;
            }

            char* end_of_section = (char*)memchr( line, ']', line_len );

            char*  section     = line + 1;
            size_t section_len = (size_t)(end_of_section - line) - 1;

            // section cannot be empty
            if( !section_len ) {
                fprintf( stderr, "%4zu | section name is empty!\n", ln );
                continue;
            }

            current_section = __ini_section( ctx, section_len, section );
        } else { // probably not a section
            char* eq = (char*)memchr( line, '=', line_len );
            if( !eq ) {
                fprintf( stderr, "%4zu | unrecognized syntax: %*.s\n", ln, (int)line_len, line );
                continue;
            }

            char*  left     = line;
            size_t left_len = (size_t)( eq - line );

            char*  right     = eq + 1;
            size_t right_len = (size_t)((line + line_len) - right);

            __ini_trim_whitespace( &left_len, &left );
            __ini_trim_whitespace( &right_len, &right );

            // every entry must have a name!
            if( !left_len ) {
                fprintf( stderr, "%4zu | field without a name!", ln );
                continue;
            }

            __ini_field( ctx, current_section, left_len, left, right_len, right );
        }
    }

    return true;
}

INI_INL
void ini_close( IniCtx* ctx ) {
    if( !ctx ) {
        return;
    }

    if( ctx->string.ptr ) {
        free( ctx->string.ptr );
    }

    if( ctx->len ) {
        for( size_t i = 0; i < ctx->len; ++i ) {
            struct IniSection* section = ctx->ptr + i;

            if( section->ptr ) {
                free( section->ptr );
            }
        }

        free( ctx->ptr );
    }

    memset( ctx, 0, sizeof(*ctx) );
}

INI_INL
size_t __ini_call_serialize( void* target, IniSerializeFN* fn, const char* fmt, ... ) {
    size_t result = 0;

    va_list va;
    va_start( va, fmt );

    result = fn( target, fmt, va );

    va_end( va );

    return result;
}

INI_INL
size_t ini_serialize( IniCtx* ctx, void* target, IniSerializeFN* fn ) {
    size_t result = 0;

    bool has_written_before = false;
    bool has_done_non_null  = false;

    for( size_t section_idx = 0; section_idx < ctx->len; ++section_idx ) {
        struct IniSection* section = ctx->ptr + section_idx;

        if( section->comment.len && section->comment.off ) {
            if( has_written_before ) {
                result += __ini_call_serialize( target, fn, "\n" );
            }

            has_written_before = true;

            result += __ini_call_serialize(
                target, fn,
                "; %.*s\n",
                (int)section->comment.len,
                ctx->string.ptr + section->comment.off );
        } else {
            if( has_written_before ) {
                result += __ini_call_serialize( target, fn, "\n" );
            }

            has_written_before = true;

            result += __ini_call_serialize( target, fn, "\n" );
        }

        if( section->name.len ) {
            has_done_non_null = true;
            result += __ini_call_serialize(
                target, fn, "[%.*s]\n",
                (int)section->name.len, ctx->string.ptr + section->name.off );
        } else {
            if( has_done_non_null ) {
                result += __ini_call_serialize( target, fn, "\n[*]\n" );
            }
        }

        size_t key_max_len = 0;
        for( size_t kv_idx = 0; kv_idx < section->len; ++kv_idx ) {
            struct IniKV* kv = section->ptr + kv_idx;

            if( kv->key.len > key_max_len ) {
                key_max_len = kv->key.len;
            }
        }

        for( size_t kv_idx = 0; kv_idx < section->len; ++kv_idx ) {
            struct IniKV* kv = section->ptr + kv_idx;

            if( !(kv->key.len && kv->key.off) ) {
                continue;
            }

            if( kv->comment.len && kv->comment.off ) {
                result += __ini_call_serialize(
                    target, fn,
                    "\n; %.*s\n",
                    (int)kv->comment.len, ctx->string.ptr + kv->comment.off );
            }

            result += __ini_call_serialize(
                target, fn, "%.*s", (int)kv->key.len, ctx->string.ptr + kv->key.off );

            if( kv->key.len < key_max_len ) {
                size_t spaces = key_max_len - kv->key.len;
                for( size_t i = 0; i < spaces; ++i ) {
                    result += __ini_call_serialize( target, fn, " " );
                }
            }

            result += __ini_call_serialize( target, fn, " = " );

            switch( kv->type ) {
                case INI_TYPE_NULL: {
                    result += __ini_call_serialize( target, fn, "null" );
                } break;
                case INI_TYPE_STRING: {
                    if( kv->value.len && kv->value.off ) {
                        char* value = ctx->string.ptr + kv->value.off;

                        bool requires_quotes = false;
                        if( isspace( value[0] ) ) {
                            requires_quotes = true;
                        }

                        if( requires_quotes ) {
                            result += __ini_call_serialize( target, fn, "\"" );
                        }

                        result += __ini_call_serialize( target, fn, "%s", value );

                        if( requires_quotes ) {
                            result += __ini_call_serialize( target, fn, "\"" );
                        }
                    } else {
                        result += __ini_call_serialize( target, fn, "null" );
                    }
                } break;
                case INI_TYPE_NUMBER: {
                    result += __ini_call_serialize( target, fn, "%f", kv->number );
                } break;
                case INI_TYPE_BOOLEAN: {
                    result += __ini_call_serialize(
                        target, fn, "%s", kv->boolean ? "true" : "false" );
                } break;
                case INI_TYPE_COUNT:
                    break;
            }

            result += __ini_call_serialize( target, fn, "\n" );
        }
    }

    return result;
}

INI_INL
size_t __ini_serialize_file( void* target, const char* fmt, va_list va ) {
    FILE* file = (FILE*)target;

    return vfprintf( file, fmt, va );
}

INI_INL
bool ini_serialize_to_file( IniCtx* ctx, const char* path, bool should_truncate ) {
    FILE* file = NULL;
    if( should_truncate ) {
        file = fopen( path, "w" );
    } else {
        file = fopen( path, "a" );
    }

    if( !file ) {
        return false;
    }

    ini_serialize( ctx, file, __ini_serialize_file );

    fclose( file );
    return true;
}


INI_INL
void __ini_search(
    IniCtx* ctx,
    const char* section, const char* key,
    bool* out_section_found, bool* out_key_found,
    size_t* out_section, size_t* out_key
) {
    bool is_null = false;
    if( section ) {
        is_null = strcmp( section, "*" ) == 0;
    } else {
        is_null = true;
    }

    if( is_null ) {
        for( size_t i = 0; i < ctx->len; ++i ) {
            struct IniSection* current = ctx->ptr + i;

            if( current->name.len ) {
                continue;
            }

            *out_section_found = true;
            *out_section       = i;
            break;
        }
    } else {
        size_t section_len = strlen( section );
        for( size_t i = 0; i < ctx->len; ++i ) {
            struct IniSection* current = ctx->ptr + i;

            if( section_len != current->name.len ) {
                continue;
            }

            char* current_name = ctx->string.ptr + current->name.off;

            if( memcmp( current_name, section, section_len ) ) {
                continue;
            }

            *out_section_found = true;
            *out_section       = i;
            break;
        }
    }

    if( !*out_section_found ) {
        *out_key_found = false;
        *out_key       = 0;
        return;
    }

    struct IniSection* p_section = ctx->ptr + *out_section;

    size_t key_len = 0;
    if( key ) {
        key_len = strlen( key );
    }
    for( size_t i = 0; i < p_section->len; ++i ) {
        struct IniKV* kv = p_section->ptr + i;

        if( kv->key.len != key_len ) {
            continue;
        }

        char* kv_key = ctx->string.ptr + kv->key.off;

        if( memcmp( key, kv_key, key_len ) ) {
            continue;
        }

        *out_key_found = true;
        *out_key       = i;
        break;
    }
}

INI_INL
void __ini_get_section_kv(
    IniCtx* ctx,
    bool section_found, bool key_found,
    size_t section_idx, size_t key_idx,
    const char* section, const char* key,
    const char* default_value,
    struct IniSection** out_section, struct IniKV** out_kv
) {
    if( section_found ) {
        *out_section = ctx->ptr + section_idx;
    } else {
        section_idx = 0;
        if( section ) {
            section_idx = __ini_section( ctx, strlen( section ), section );
        } else {
            section_idx = __ini_section( ctx, 0, 0 );
        }

        *out_section = ctx->ptr + section_idx;
    }

    if( key_found ) {
        *out_kv = (*out_section)->ptr + key_idx;
    } else {
        size_t idx = __ini_field(
            ctx, section_idx,
            strlen( key ), (char*)key,
            default_value ? strlen(default_value) : 0, (char*)default_value );

        *out_kv = (*out_section)->ptr + idx;
    }
}

INI_INL
bool __ini_get(
    IniCtx* ctx,
    bool dont_create,
    const char* txt_section,
    const char* txt_key,
    const char* default_value,
    const char* comment,
    bool* was_found,
    struct IniKV** out_kv
) {
    bool section_found, key_found;
    section_found = key_found = false;
    
    size_t section_idx, key_idx;
    section_idx = key_idx = 0;

    const char* section_name = txt_section;
    const char* key_name     = txt_key;

    __ini_search(
        ctx, section_name, key_name,
        &section_found, &key_found,
        &section_idx, &key_idx );

    if( !(section_found && key_found) ) {
        *was_found = false;
        if( dont_create ) {
            return false;
        }
    } else {
        *was_found = true;
    }

    struct IniSection* section = NULL;
    struct IniKV*      kv      = NULL;

    __ini_get_section_kv(
        ctx,
        section_found, key_found,
        section_idx, key_idx,
        section_name, key_name,
        default_value,
        &section, &kv );

    if( comment ) {
        kv->comment = __ini_str( ctx, "%s", comment );
    }

    *out_kv = kv;

    return true;
}

INI_INL
const char* __ini_read(
    IniCtx* ctx,
    const char* txt_section,
    const char* txt_key,
    struct IniReadSettings settings
) {
    if( !txt_key ) {
        if( settings.is_found ) {
            *settings.is_found = false;
        }
        if( settings.is_type_correct ) {
            *settings.is_type_correct = false;
        }
        return "";
    }

    struct IniKV* kv = NULL;

    bool found = false;
    if( __ini_get(
        ctx, settings.dont_create,
        txt_section, txt_key,
        settings.default_value, settings.comment,
        &found,
        &kv
    ) ) {
        if( settings.is_found ) {
            *settings.is_found = found;
        }
    } else {
        if( settings.is_found ) {
            *settings.is_found = false;
        }
        if( settings.is_type_correct ) {
            *settings.is_type_correct = false;
        }
        return "";
    }

    const char* value = "";

    switch( kv->type ) {
        case INI_TYPE_STRING: {
            if( settings.is_type_correct ) {
                *settings.is_type_correct = true;
            }

            if( kv->value.off ) {
                value = ctx->string.ptr + kv->value.off;
            }
        } break;

        case INI_TYPE_NULL:
        case INI_TYPE_NUMBER:
        case INI_TYPE_BOOLEAN:
        case INI_TYPE_COUNT: {
            if( settings.is_type_correct ) {
                *settings.is_type_correct = false;
            }

            switch( kv->type ) {
                case INI_TYPE_NULL: {
                    value = "";
                } break;
                case INI_TYPE_NUMBER: {
                    value = ctx->string.ptr + kv->value.off;
                } break;
                case INI_TYPE_BOOLEAN: {
                    if( kv->boolean ) {
                        value = "true";
                    } else {
                        value = "false";
                    }
                } break;

                case INI_TYPE_STRING:
                case INI_TYPE_COUNT:
                    break;
            }
        } break;
    }

    return value;
}
INI_INL
double __ini_read_number(
    IniCtx* ctx, const char* txt_section, const char* txt_key, struct IniReadSettings settings
) {
    if( !txt_key ) {
        if( settings.is_found ) {
            *settings.is_found = false;
        }
        if( settings.is_type_correct ) {
            *settings.is_type_correct = false;
        }
        return 0.0;
    }

    struct IniKV* kv = NULL;

    bool found = false;
    if( __ini_get(
        ctx, settings.dont_create,
        txt_section, txt_key,
        settings.default_value, settings.comment,
        &found,
        &kv
    ) ) {
        if( settings.is_found ) {
            *settings.is_found = found;
        }
    } else {
        if( settings.is_found ) {
            *settings.is_found = false;
        }
        if( settings.is_type_correct ) {
            *settings.is_type_correct = false;
        }
        return 0.0;
    }

    double value = 0.0;

    switch( kv->type ) {
        case INI_TYPE_NULL: {
            if( settings.is_type_correct ) {
                *settings.is_type_correct = false;
            }
        } break;
        case INI_TYPE_STRING: {
            if( settings.is_type_correct ) {
                *settings.is_type_correct = false;
            }

            char* endptr = NULL;
            value = strtod( ctx->string.ptr + kv->value.off, &endptr );
        } break;
        case INI_TYPE_NUMBER: {
            if( settings.is_type_correct ) {
                *settings.is_type_correct = true;
            }

            value = kv->number;
        } break;
        case INI_TYPE_BOOLEAN: {
            if( settings.is_type_correct ) {
                *settings.is_type_correct = false;
            }

            value = kv->boolean ? 1.0 : 0.0;
        } break;
        case INI_TYPE_COUNT: {
            if( settings.is_type_correct ) {
                *settings.is_type_correct = false;
            }
        } break;
    }

    return value;
}
INI_INL
bool __ini_read_bool(
    IniCtx* ctx, const char* txt_section, const char* txt_key, struct IniReadSettings settings
) {
    if( !txt_key ) {
        if( settings.is_found ) {
            *settings.is_found = false;
        }
        if( settings.is_type_correct ) {
            *settings.is_type_correct = false;
        }
        return false;
    }

    struct IniKV* kv = NULL;

    bool found = false;
    if( __ini_get(
        ctx, settings.dont_create,
        txt_section, txt_key,
        settings.default_value, settings.comment,
        &found,
        &kv
    ) ) {
        if( settings.is_found ) {
            *settings.is_found = found;
        }
    } else {
        if( settings.is_found ) {
            *settings.is_found = false;
        }
        if( settings.is_type_correct ) {
            *settings.is_type_correct = false;
        }
        return false;
    }

    bool value = false;

    switch( kv->type ) {
        case INI_TYPE_NULL: {
            if( settings.is_type_correct ) {
                *settings.is_type_correct = false;
            }
        } break;
        case INI_TYPE_STRING: {
            if( settings.is_type_correct ) {
                *settings.is_type_correct = false;
            }

            value = kv->value.off != 0;
        } break;
        case INI_TYPE_NUMBER: {
            if( settings.is_type_correct ) {
                *settings.is_type_correct = true;
            }

            value = kv->number != 0.0;
        } break;
        case INI_TYPE_BOOLEAN: {
            if( settings.is_type_correct ) {
                *settings.is_type_correct = false;
            }

            value = kv->boolean;
        } break;
        case INI_TYPE_COUNT: {
            if( settings.is_type_correct ) {
                *settings.is_type_correct = false;
            }
        } break;
    }

    return value;
}

INI_INL
void ini_write_va(
    IniCtx* ctx, const char* section_name, const char* key_name, const char* value, va_list va
) {
    if( !key_name ) {
        return;
    }
    size_t key_len = strlen( key_name );
    if( !key_len ) {
        return;
    }

    size_t section_idx =
        __ini_section( ctx, section_name ? strlen(section_name) : 0, section_name );

    struct IniSection* section = ctx->ptr + section_idx;

    bool   is_found = false;
    size_t index    = 0;

    for( size_t i = 0; i < section->len; ++i ) {
        struct IniKV* kv = section->ptr + i;

        if( key_len != kv->key.len ) {
            continue;
        }

        if( memcmp( key_name, ctx->string.ptr + kv->key.off, key_len ) ) {
            continue;
        }

        is_found = true;
        index    = i;
        break;
    }

    if( is_found ) {
        struct IniKV* kv = section->ptr + index;

        if( value ) {
            struct IniStr string = __ini_str_va( ctx, value, va );
            __ini_assign_value( ctx, kv, string.len, ctx->string.ptr + string.off, true );
        } else {
            __ini_assign_value( ctx, kv, 0, 0, false );
        }

        return;
    }

    struct IniKV kv;
    memset( &kv, 0, sizeof(kv) );

    kv.key = __ini_str( ctx, "%s", key_name );
    if( value ) {
        struct IniStr string = __ini_str_va( ctx, value, va );
        __ini_assign_value( ctx, &kv, string.len, ctx->string.ptr + string.off, true );
    } else {
        __ini_assign_value( ctx, &kv, 0, 0, false );
    }

    PUSH( section, kv );
}

INI_INL
void ini_write(
    IniCtx* ctx, const char* section, const char* key, const char* value, ...
) {
    va_list va;
    va_start( va, value );

    ini_write_va( ctx, section, key, value, va );

    va_end( va );
}

INI_INL
void ini_set_section_comment_va(
    IniCtx* ctx, const char* section, const char* comment, va_list va
) {
    bool   is_found    = false;
    size_t section_idx = 0;

    bool is_null = false;
    if( section ) {
        is_null = (strcmp( section, "*" ) == 0) || !strlen(section);
    } else {
        is_null = true;
    }

    if( is_null ) {
        for( size_t i = 0; i < ctx->len; ++i ) {
            struct IniSection* current = ctx->ptr + i;

            if( current->name.len ) {
                continue;
            }

            is_found    = true;
            section_idx = i;
            break;
        }
    } else {
        size_t section_len = strlen( section );

        for( size_t i = 0; i < ctx->len; ++i ) {
            struct IniSection* current = ctx->ptr + i;

            if( current->name.len != section_len ) {
                continue;
            }

            char* name = ctx->string.ptr + current->name.off;
            if( memcmp( name, section, section_len ) ) {
                continue;
            }

            is_found    = true;
            section_idx = i;
            break;
        }
    }

    if( !is_found ) {
        return;
    }

    struct IniSection* sect = ctx->ptr + section_idx;

    sect->comment = __ini_str_va( ctx, comment, va );
}

INI_INL
void ini_set_section_comment( IniCtx* ctx, const char* section, const char* comment, ... ) {
    va_list va;
    va_start( va, comment );

    ini_set_section_comment_va( ctx, section, comment, va );

    va_end( va );
}

INI_INL
void ini_set_comment_va(
    IniCtx* ctx, const char* section, const char* key, const char* comment, va_list va
) {
    if( !key ) {
        return;
    }

    size_t kv_len = strlen( key );
    if( !kv_len ) {
        return;
    }

    bool   is_found    = false;
    size_t section_idx = 0;

    bool is_null = false;
    if( section ) {
        is_null = (strcmp( section, "*" ) == 0) || !strlen(section);
    } else {
        is_null = true;
    }

    if( is_null ) {
        for( size_t i = 0; i < ctx->len; ++i ) {
            struct IniSection* current = ctx->ptr + i;

            if( current->name.len ) {
                continue;
            }

            is_found    = true;
            section_idx = i;
            break;
        }
    } else {
        size_t section_len = strlen( section );

        for( size_t i = 0; i < ctx->len; ++i ) {
            struct IniSection* current = ctx->ptr + i;

            if( current->name.len != section_len ) {
                continue;
            }

            char* name = ctx->string.ptr + current->name.off;
            if( memcmp( name, section, section_len ) ) {
                continue;
            }

            is_found    = true;
            section_idx = i;
            break;
        }
    }

    if( !is_found ) {
        return;
    }

    struct IniSection* sect = ctx->ptr + section_idx;

    is_found      = false;
    size_t kv_idx = 0;

    for( size_t i = 0; i < sect->len; ++i ) {
        struct IniKV* kv = sect->ptr + i;

        if( kv_len != kv->key.len ) {
            continue;
        }

        char* name = ctx->string.ptr + kv->key.off;
        if( memcmp( name, key, kv_len ) ) {
            continue;
        }

        is_found = true;
        kv_idx   = i;
        break;
    }

    if( !is_found ) {
        return;
    }

    struct IniKV* kv = sect->ptr + kv_idx;

    kv->comment = __ini_str_va( ctx, comment, va );
}

INI_INL
void ini_set_comment(
    IniCtx* ctx, const char* section, const char* key, const char* comment, ...
) {
    va_list va;
    va_start( va, comment );

    ini_set_comment_va( ctx, section, key, comment, va );

    va_end( va );
}


INI_INL
const char* string_from_ini_type( enum IniType type ) {
    switch( type ) {
        case INI_TYPE_NULL    : return "null";
        case INI_TYPE_STRING  : return "string";
        case INI_TYPE_NUMBER  : return "number";
        case INI_TYPE_BOOLEAN : return "boolean";
        case INI_TYPE_COUNT   : break;
    }
    return "(unknown)";
}

#undef RESERVE
#undef PUSH
#undef APPEND

#endif /* header guard */
