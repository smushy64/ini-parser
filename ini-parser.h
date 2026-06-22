#if !defined(INI_PARSER_H)
#define INI_PARSER_H
/**
 * @file   ini-parser.h
 * @brief  .ini parser written in C.
 * @author Alicia D. Amarilla (smushyaa@gmail.com)
 * @date   June 18, 2026
*/
#include <stdarg.h>  // va_list, va_copy, va_start, va_end
#include <stddef.h>  // size_t
#include <stdbool.h> // bool, true, false
#include <stdlib.h>  // calloc, realloc, free, strtol, strtod
#include <string.h>  // memcpy, memset, strchr, strpbrk
#include <stdio.h>   // vsnprintf, vfprintf
#include <assert.h>  // assert

#undef INIPARSE_FMTFUNC
#undef INIPARSE_INLINE
#if __cplusplus
    #define STRUCT(x, ...)  x{__VA_ARGS__}
    #define INIPARSE_INLINE inline
#else
    #define STRUCT(x, ...)  (struct x){__VA_ARGS__}
    #define INIPARSE_INLINE static inline
#endif

#if __GNUC__
    #define INIPARSE_FMTFUNC(string_index, first_to_check) \
        __attribute__((format (printf, string_index, first_to_check)))
#else
    #define INIPARSE_FMTFUNC(...)
#endif

#define INIPARSE_ESCAPE_CH "\n\\\""

/// @brief Parser configuration flags.
enum IniParserFlag {
    /// @brief If true, fails to serialize if file already exists.
    INI_PARSER_FLAG_SERIALIZE_FAIL_IF_EXISTS,
    /// @brief If true, appends to existing file when serializing.
    INI_PARSER_FLAG_SERIALIZE_APPEND_IF_EXISTS,
    /// @brief If true, print errors when deserializing. Default value is true.
    INI_PARSER_FLAG_DESERIALIZE_PRINT_ERRORS,
};

/// @brief Parser field types.
enum IniParserType {
    /// @brief Null field.
    INI_PARSER_TYPE_NULL,
    /// @brief String field.
    INI_PARSER_TYPE_STRING,
    /// @brief Boolean field.
    INI_PARSER_TYPE_BOOL,
    /// @brief Integer field.
    INI_PARSER_TYPE_INTEGER,
    /// @brief Float field.
    INI_PARSER_TYPE_FLOAT,
};

/// @brief Streaming function prototype.
/// @param[in] target Pointer to streaming target.
/// @param     n      Number of bytes pointed to by @c bytes.
/// @param[in] bytes  Pointer to bytes.
/// @return Number of bytes streamed.
typedef size_t IniParserStreamFn(void *target, size_t n, const void *bytes);

/// @brief Allocator function prototype.
/// @param     n   Number of bytes to allocate.
/// @param[in] ctx Pointer to allocator context.
/// @return Pointer to memory.
typedef void *IniParserAllocatorAllocFn(size_t n, void *ctx);
/// @brief Allocator realloc function prototype.
/// @param[in] ptr   Pointer to reallocate.
/// @param     oldsz Size of memory pointed to by @c ptr.
/// @param     newsz New size of memory, must be larger than @c oldsz.
/// @param[in] ctx   Pointer to allocator context.
/// @return Pointer to memory.
typedef void *IniParserAllocatorReallocFn(void *ptr, size_t oldsz, size_t newsz, void *ctx);
/// @brief Allocator free function prototype.
/// @param[in] ptr Pointer to free.
/// @param     sz  Size of memory.
/// @parma[in] ctx Pointer to allocator context.
typedef void  IniParserAllocatorFreeFn(void *ptr, size_t sz, void *ctx);

/// @brief Allocator.
struct IniParserAllocator {
    /// @brief Pointer to alloc function.
    IniParserAllocatorAllocFn   *pfn_alloc;
    /// @brief Pointer to realloc function.
    /// @note Not required, if missing will use a combo of alloc and free.
    IniParserAllocatorReallocFn *pfn_realloc;
    /// @brief Pointer to free function.
    IniParserAllocatorFreeFn    *pfn_free;

    /// @brief Pointer to allocator context, if needed.
    void *ctx;
};

/// @brief String buffer.
struct IniParserStringBuffer {
    /// @brief Capacity of buffer.
    unsigned int cap;
    /// @brief Length of buffer.
    unsigned int len;
    /// @brief Pointer to start of buffer.
    char        *ptr;
};

/// @brief Parser field.
struct IniParserField {
    /// @brief Offset of field name.
    unsigned int name;
    /// @brief Offset of field value.
    unsigned int value_raw;
    /// @brief Offset of field comment.
    unsigned int comment;

    /// @brief Type of field.
    enum IniParserType type : 16;
    /// @brief If field has escapable characters in value.
    bool has_escapable_ch : 1;

    union {
        /// @brief Parsed boolean value.
        /// @note Only valid when @c type is @c INI_pARSER_TYPE_BOOL
        bool   t_bool;
        /// @brief Parsed integer value.
        /// @note Only valid when @c type is @c INI_pARSER_TYPE_INTEGER
        long   t_integer;
        /// @brief Parsed float value.
        /// @note Only valid when @c type is @c INI_pARSER_TYPE_FLOAT
        double t_float;
    };
};

/// @brief Parser section.
struct IniParserSection {
    /// @brief Offset of section name.
    unsigned int name;
    /// @brief Offset of section comment.
    unsigned int comment;

    /// @brief Field capacity of section.
    unsigned int           cap;
    /// @brief Number of fields in section.
    unsigned int           len;
    /// @brief Pointer to start of field buffer.
    struct IniParserField *ptr;
};

/// @brief Parser context.
struct IniParserContext {
    /// @brief Allocator.
    struct IniParserAllocator allocator;

    /// @brief String buffers.
    struct IniParserStringBuffer str, tmp;

    /// @brief Section capacity.
    unsigned int             cap;
    /// @brief Number of sections.
    unsigned int             len;
    /// @brief Pointer to start of section buffer.
    struct IniParserSection *ptr;

    /// @brief Current section and field.
    int section, field;

    /// @brief Bitflags.
    unsigned int flags;
};

/// @brief Initialize parser.
/// @param[in] ctx Pointer to context.
INIPARSE_INLINE
void ini_parser_begin(struct IniParserContext *ctx);
/// @brief Initialize parser with custom allocator.
/// @param[in] ctx       Pointer to context.
/// @param[in] allocator Pointer to custom allocator.
INIPARSE_INLINE
void ini_parser_begin_with_allocator(
    struct IniParserContext *ctx, struct IniParserAllocator *allocator);
/// @brief Cleanup parser.
/// @note Do not call any other ini_parser functions after this.
/// @param[in] ctx Pointer to context.
INIPARSE_INLINE
void ini_parser_end(struct IniParserContext *ctx);

/// @brief Begin a new or existing section.
/// @details
/// Use begin and end to define a section.
/// When reading values, begin and end determine which section values are read from.
/// When writing values, begin and end determine which section values are written to.
/// @note This function is not necessary if no sections are used.
/// @param[in] ctx     Pointer to context.
/// @param[in] section Format string for section name.
/// @param     ...     Format string arguments.
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 3)
void ini_parser_begin_section(struct IniParserContext *ctx, const char *section, ...);
/// @brief Begin a new or existing section.
/// @details
/// Use begin and end to define a section.
/// When reading values, begin and end determine which section values are read from.
/// When writing values, begin and end determine which section values are written to.
/// @note This function is not necessary if no sections are used.
/// @param[in] ctx     Pointer to context.
/// @param[in] section Format string for section name.
/// @param[in] va      Format string arguments.
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 0)
void ini_parser_begin_section_va(struct IniParserContext *ctx, const char *section, va_list va);
/// @brief End current section.
/// @note Should always be called before the next call to begin section.
/// @param[in] ctx Pointer to context.
INIPARSE_INLINE
void ini_parser_end_section(struct IniParserContext *ctx);

/// @brief Begin a new or existing field.
/// @details
/// Only needed for writing fields.
/// @note Must always be followed by end field.
/// @param[in] ctx  Pointer to context.
/// @param[in] name Format string for field name.
/// @param     ...  Format string arguments.
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 3)
void ini_parser_begin_field(struct IniParserContext *ctx, const char *name, ...);
/// @brief Begin a new or existing field.
/// @details
/// Only needed for writing fields.
/// @note Must always be followed by end field.
/// @param[in] ctx  Pointer to context.
/// @param[in] name Format string for field name.
/// @param     va   Format string arguments.
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 0)
void ini_parser_begin_field_va(struct IniParserContext *ctx, const char *name, va_list va); 
/// @brief End field.
/// @param[in] ctx Pointer to context.
INIPARSE_INLINE
void ini_parser_end_field(struct IniParserContext *ctx);

/// @brief Write a new value to current field.
/// @details
/// Value gets parsed and assigned a type.
/// If value cannot be parsed, it is assigned INI_PARSER_TYPE_STRING.
/// If value is empty or NULL, it is assigned INI_PARSER_TYPE_NULL.
/// @note Must be wrapped in begin_field and end_field calls!
/// @param[in] ctx   Pointer to context.
/// @param[in] value Format string for value.
/// @param     ...   Format string arguments.
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 3)
void ini_parser_value(struct IniParserContext *ctx, const char *value, ...);
/// @brief Write a new value to current field.
/// @details
/// Value gets parsed and assigned a type.
/// If value cannot be parsed, it is assigned INI_PARSER_TYPE_STRING.
/// If value is empty or NULL, it is assigned INI_PARSER_TYPE_NULL.
/// @note Must be wrapped in begin_field and end_field calls!
/// @param[in] ctx   Pointer to context.
/// @param[in] value Format string for value.
/// @param[in] va    Format string arguments.
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 0)
void ini_parser_value_va(struct IniParserContext *ctx, const char *value, va_list va); 

/// @brief Write a comment to current section or field.
/// @note If outside of begin_field and end_field, writes a
/// comment for the current section.
/// Otherwise, writes a comment for the current field.
/// @param[in] ctx     Pointer to context.
/// @param[in] comment Format string for comment.
/// @param     ...     Format string arguments.
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 3)
void ini_parser_comment(struct IniParserContext *ctx, const char *comment, ...);
/// @brief Write a comment to current section or field.
/// @note If outside of begin_field and end_field, writes a
/// comment for the current section.
/// Otherwise, writes a comment for the current field.
/// @param[in] ctx     Pointer to context.
/// @param[in] comment Format string for comment.
/// @param[in] va      Format string arguments.
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 0)
void ini_parser_comment_va(struct IniParserContext *ctx, const char *comment, va_list va);

/// @brief Check if field exists in the current section.
/// @param[in] ctx  Pointer to context.
/// @param[in] name Format string for name of field.
/// @param     ...  Format string arguments.
/// @return True if field exists or false if it doesn't.
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 3)
bool ini_parser_exists(struct IniParserContext *ctx, const char *name, ...);
/// @brief Check if field exists in the current section.
/// @param[in] ctx  Pointer to context.
/// @param[in] name Format string for name of field.
/// @param[in] va   Format string arguments.
/// @return True if field exists or false if it doesn't.
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 0)
bool ini_parser_exists_va(struct IniParserContext *ctx, const char *name, va_list va);

/// @brief Read field as a string.
/// @param[in] ctx  Pointer to context.
/// @param[in] name Format string for name of field.
/// @param     ...  Format string arguments.
/// @return Field value as a string or empty string if it doesn't exist.
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 3)
const char *ini_parser_read_string(struct IniParserContext *ctx, const char *name, ...);
/// @brief Read field as a string.
/// @param[in] ctx  Pointer to context.
/// @param[in] name Format string for name of field.
/// @param[in] va   Format string arguments.
/// @return Field value as a string or empty string if it doesn't exist.
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 0)
const char *ini_parser_read_string_va(struct IniParserContext *ctx, const char *name, va_list va);

/// @brief Read field as a bool.
/// @param[in] ctx  Pointer to context.
/// @param[in] name Format string for name of field.
/// @param     ...  Format string arguments.
/// @return Field value.
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 3)
bool ini_parser_read_bool(struct IniParserContext *ctx, const char *name, ...);
/// @brief Read field as a bool.
/// @param[in] ctx  Pointer to context.
/// @param[in] name Format string for name of field.
/// @param[in] va   Format string arguments.
/// @return Field value.
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 0)
bool ini_parser_read_bool_va(struct IniParserContext *ctx, const char *name, va_list va);

/// @brief Read field as an integer.
/// @param[in] ctx  Pointer to context.
/// @param[in] name Format string for name of field.
/// @param     ...  Format string arguments.
/// @return Field value.
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 3)
long ini_parser_read_integer(struct IniParserContext *ctx, const char *name, ...);
/// @brief Read field as an integer.
/// @param[in] ctx  Pointer to context.
/// @param[in] name Format string for name of field.
/// @param[in] va   Format string arguments.
/// @return Field value.
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 0)
long ini_parser_read_integer_va(struct IniParserContext *ctx, const char *name, va_list va);

/// @brief Read field as a float.
/// @param[in] ctx  Pointer to context.
/// @param[in] name Format string for name of field.
/// @param     ...  Format string arguments.
/// @return Field value.
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 3)
double ini_parser_read_float(struct IniParserContext *ctx, const char *name, ...);
/// @brief Read field as a float.
/// @param[in] ctx  Pointer to context.
/// @param[in] name Format string for name of field.
/// @param[in] va   Format string arguments.
/// @return Field value.
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 0)
double ini_parser_read_float_va(struct IniParserContext *ctx, const char *name, va_list va);

/// @brief Write to parser context flags.
/// @param[in] ctx   Pointer to context.
/// @param     flag  Which flag to write to.
/// @param     value Value to write to flag.
/// @return Value written to flag.
INIPARSE_INLINE
bool ini_parser_flag_write(struct IniParserContext *ctx, enum IniParserFlag flag, bool value);
/// @brief Read from parser context flags.
/// @param[in] ctx   Pointer to context.
/// @param     flag  Which flag to read.
/// @return Value from flag.
INIPARSE_INLINE
bool ini_parser_flag_read(struct IniParserContext *ctx, enum IniParserFlag flag);

/// @brief Serialize the current context.
/// @param[in] ctx    Pointer to context.
/// @param[in] stream Pointer to streaming function.
/// @param[in] target Pointer to target of streaming function.
/// @return Number of bytes streamed.
INIPARSE_INLINE
size_t ini_parser_serialize_stream(
    struct IniParserContext *ctx, IniParserStreamFn *stream, void *target);
/// @brief Serialize the current context to a file.
/// @param[in] ctx Pointer to context.
/// @param[in] fp  Path of file to write to.
/// @return If file was opened and written to successfully.
INIPARSE_INLINE
bool ini_parser_serialize_file_path(struct IniParserContext *ctx, const char *fp);
/// @brief Serialize the current context to a file.
/// @param[in] ctx Pointer to context.
/// @param[in] f   File handle.
INIPARSE_INLINE
void ini_parser_serialize_file(struct IniParserContext *ctx, FILE *f);

/// @brief Deserialize ini from byte buffer.
/// @note To avoid reallocating the byte buffer, include null-terminator in @c n.
/// @param[in] ctx   Pointer to the context.
/// @param     n     Number of bytes in buffer.
/// @param[in] bytes Pointer to byte buffer.
/// @return True if there were no errors encountered.
INIPARSE_INLINE
bool ini_parser_deserialize(struct IniParserContext *ctx, size_t n, const void *bytes);
/// @brief Deserialize ini from file.
/// @param[in] ctx Pointer to the context.
/// @param[in] fp  Path of file.
/// @return True if file was opened/read and there were no errors encountered.
INIPARSE_INLINE
bool ini_parser_deserialize_file_path(struct IniParserContext *ctx, const char *fp);
/// @brief Deserialize ini from file.
/// @param[in] ctx Pointer to the context.
/// @param[in] f   File handle.
/// @return True if file was read and there were no errors encountered.
INIPARSE_INLINE
bool ini_parser_deserialize_file(struct IniParserContext *ctx, FILE *f);

// NOTE(alicia): implementation -----------------------------------------------------------

INIPARSE_INLINE
void *__ini_parser_alloc(struct IniParserAllocator *a, size_t n) {
    if(a->pfn_alloc) {
        return a->pfn_alloc(n, a->ctx);
    } else {
        return calloc(1, n);
    }
}

INIPARSE_INLINE
void __ini_parser_free(struct IniParserAllocator *a, void *ptr, size_t sz) {
    if(a->pfn_free) {
        a->pfn_free(ptr, sz, a->ctx);
    } else {
        free(ptr);
    }
}

INIPARSE_INLINE
void *__ini_parser_realloc(struct IniParserAllocator *a, void *ptr, size_t oldsz, size_t newsz) {
    if(a->pfn_realloc) {
        return a->pfn_realloc(ptr, oldsz, newsz, a->ctx);
    } else {
        void *new_ptr = __ini_parser_alloc(a, newsz);
        memcpy(new_ptr, ptr, oldsz);
        __ini_parser_free(a, ptr, oldsz);
        return new_ptr;
    }
}

INIPARSE_INLINE
void __ini_parser_reserve(
    struct IniParserAllocator *a, size_t stride,
    unsigned int len, unsigned int *cap, void **ptr, unsigned int min
) {
    if(*ptr) {
        if((*cap - len) < min) {
            size_t oldsz = stride * *cap;

            size_t delta = min - (*cap - len);
            delta += 16 - (delta % 16);
            delta += 16;

            size_t newsz = stride * (*cap + delta);

            *ptr = __ini_parser_realloc(a, *ptr, oldsz, newsz);
            *cap += delta;
        }
    } else {
        size_t sz = min;
        sz = sz < 16 ? 16 : sz;
        unsigned int new_cap = sz;
        sz = stride * sz;

        *ptr = __ini_parser_alloc(a, sz);
        *cap = new_cap;
    }
}

#define reserve(a, b, min) \
    __ini_parser_reserve((a), sizeof((b)->ptr[0]), (b)->len, &(b)->cap, (void **)&(b)->ptr, (min))

INIPARSE_INLINE
unsigned int __ini_parser_push_string(
    struct IniParserAllocator *a, struct IniParserStringBuffer *b,
    const char *str, size_t *opt_len, ...
) {
    size_t len = opt_len ? *opt_len : strlen(str);

    unsigned int offset = b->len;

    reserve(a, b, len + 1);
    memcpy(b->ptr + b->len, str, len);
    b->len += len + 1;

    return offset;
}

INIPARSE_INLINE
unsigned int __ini_parser_push_char(
    struct IniParserAllocator *a, struct IniParserStringBuffer *b,
    char ch
) {
    size_t len = 1;
    unsigned int result = __ini_parser_push_string(a, b, &ch, &len);
    // remove null terminator
    b->len--;

    return result;
}

INIPARSE_INLINE INIPARSE_FMTFUNC(3, 0)
unsigned int __ini_parser_fmt_va(
    struct IniParserAllocator *a, struct IniParserStringBuffer *b,
    const char *fmt, va_list va
) {
    unsigned int offset = b->len;

    va_list va2;
    va_copy(va2, va);

    int len = vsnprintf(NULL, 0, fmt, va2);

    va_end(va2);

    if(!len) {
        return 0;
    }

    reserve(a, b, len + 1);

    char *ptr = b->ptr + b->len;
    vsnprintf(ptr, len + 1, fmt, va);

    // include null
    b->len += len + 1;

    return offset;
}

INIPARSE_INLINE INIPARSE_FMTFUNC(3, 4)
unsigned int __ini_parser_fmt(
    struct IniParserAllocator *a, struct IniParserStringBuffer *b,
    const char *fmt, ...
) {
    va_list va;
    va_start(va, fmt);
    unsigned int offset = __ini_parser_fmt_va(a, b, fmt, va);
    va_end(va);

    return offset;
}

#define pushc(a, b, c) \
    __ini_parser_push_char((a), (b), (c))

#define push(a, b, s, ...) \
    __ini_parser_push_string((a), (b), (s) __VA_OPT__(,) __VA_ARGS__ , NULL)

#define pushn(a, b, s, len) \
    __ini_parser_push_string((a), (b), (s), (size_t[]){(len)})

#define push_fmt_va(a, b, fmt, va) \
    __ini_parser_fmt_va((a), (b), fmt, va)

#define push_fmt(a, b, fmt, ...) \
    __ini_parser_fmt((a), (b), fmt __VA_OPT__(,) __VA_ARGS__ )

INIPARSE_INLINE
struct IniParserSection *__ini_parser_current_section(struct IniParserContext *ctx) {
    // if we've opened a section, use that
    if(ctx->section >= 0) {
        return ctx->ptr + ctx->section;
    }

    // otherwise create an empty section
    for(unsigned int i = 0; i < ctx->len; ++i) {
        struct IniParserSection *s = ctx->ptr + i;

        if(!s->name) {
            ctx->section = i;
            return s;
        }
    }

    reserve(&ctx->allocator, ctx, 1);
    ctx->len++;
    ctx->section = 0;

    return ctx->ptr + ctx->section;
}

INIPARSE_INLINE
void ini_parser_begin(struct IniParserContext *ctx) {
    ini_parser_begin_with_allocator(ctx, NULL);
}
INIPARSE_INLINE
void ini_parser_begin_with_allocator(
    struct IniParserContext *ctx, struct IniParserAllocator *allocator
) {
    memset(ctx, 0, sizeof(*ctx));

    if(allocator) {
        memcpy(&ctx->allocator, allocator, sizeof(*allocator));
    }

    struct IniParserAllocator *a = &ctx->allocator;

    reserve(a, ctx, 16);

    reserve(a, &ctx->str, 16);
    reserve(a, &ctx->tmp, 16);

    // force 0 to always mean null
    ctx->str.len = ctx->tmp.len = 1;

    ctx->section = ctx->field = -1;

    // initialize flags that start true
    ini_parser_flag_write(ctx, INI_PARSER_FLAG_DESERIALIZE_PRINT_ERRORS, true);
}

INIPARSE_INLINE
void ini_parser_end(struct IniParserContext *ctx) {
    struct IniParserAllocator *a = &ctx->allocator;

    if(ctx->ptr) {
        for(unsigned int i = 0; i < ctx->len; ++i) {
            struct IniParserSection *s = ctx->ptr + i;

            if(s->ptr) {
                __ini_parser_free(a, s->ptr, sizeof(s->ptr[0]) * s->cap);
            }
        }

        __ini_parser_free(a, ctx->ptr, sizeof(ctx->ptr[0]) * ctx->cap);
    }

    if(ctx->str.ptr) {
        __ini_parser_free(a, ctx->str.ptr, ctx->str.cap);
    }
    if(ctx->tmp.ptr) {
        __ini_parser_free(a, ctx->tmp.ptr, ctx->tmp.cap);
    }

    memset(ctx, 0, sizeof(*ctx));
}

INIPARSE_INLINE INIPARSE_FMTFUNC(2, 3)
void ini_parser_begin_section(struct IniParserContext *ctx, const char *section, ...) {
    va_list va;
    va_start(va, section);
    ini_parser_begin_section_va(ctx, section, va);
    va_end(va);
}
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 0)
void ini_parser_begin_section_va(struct IniParserContext *ctx, const char *section, va_list va) {
    ctx->tmp.len = 1;
    unsigned int off  = section ? push_fmt_va(&ctx->allocator, &ctx->tmp, section, va) : 0;
    const char *sname = off ? ctx->tmp.ptr + off : NULL;

    // search for section
    int soff = -1;
    for(unsigned int i = 0; i < ctx->len; ++i) {
        struct IniParserSection *s = ctx->ptr + i;

        const char *cname = s->name ? ctx->str.ptr + s->name : NULL;

        if(!sname) {
            if(!cname) {
                soff = i;
            }
        } else {
            if(!cname) {
                continue;
            }

            if(strcmp(sname, cname) == 0) {
                soff = i;
            }
        }

        if(soff >= 0) {
            break;
        }
    }

    if(soff < 0) {
        // create new section
        reserve(&ctx->allocator, ctx, 1);

        struct IniParserSection snew;
        memset(&snew, 0, sizeof(snew));

        snew.name = push(&ctx->allocator, &ctx->str, sname);

        soff = ctx->len++;
        ctx->ptr[soff] = snew;
    }

    // set current section to existing section
    ctx->section = soff;
}
INIPARSE_INLINE
void ini_parser_end_section(struct IniParserContext *ctx) {
    ctx->section = -1;
}

INIPARSE_INLINE INIPARSE_FMTFUNC(2, 3)
void ini_parser_begin_field(struct IniParserContext *ctx, const char *name, ...) {
    va_list va;
    va_start(va, name);
    ini_parser_begin_field_va(ctx, name, va);
    va_end(va);
}
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 0)
void ini_parser_begin_field_va(struct IniParserContext *ctx, const char *name, va_list va) {
    ctx->tmp.len = 1;
    unsigned int off  = name ? push_fmt_va(&ctx->allocator, &ctx->tmp, name, va) : 0;
    const char *fname = off ? ctx->tmp.ptr + off : "_";

    // search for field
    struct IniParserSection *s = __ini_parser_current_section(ctx);
    int foff = -1;
    for(unsigned int i = 0; i < s->len; ++i) {
        struct IniParserField *f = s->ptr + i;

        const char *cname = f->name ? ctx->str.ptr + f->name : NULL;

        if(!fname) {
            if(!cname) {
                foff = i;
            }
        } else {
            if(!cname) {
                continue;
            }

            if(strcmp(fname, cname) == 0) {
                foff = i;
            }
        }

        if(foff >= 0) {
            break;
        }
    }

    if(foff < 0) {
        // create new field
        reserve(&ctx->allocator, s, 1);

        struct IniParserField fnew;
        memset(&fnew, 0, sizeof(fnew));

        fnew.name = push(&ctx->allocator, &ctx->str, fname);

        foff = s->len++;
        s->ptr[foff] = fnew;
    }

    // set current field to existing field
    ctx->field = foff;
}
INIPARSE_INLINE
void ini_parser_end_field(struct IniParserContext *ctx) {
    ctx->field = -1;
}

INIPARSE_INLINE INIPARSE_FMTFUNC(2, 3)
void ini_parser_value(struct IniParserContext *ctx, const char *value, ...) {
    va_list va;
    va_start(va, value);
    ini_parser_value_va(ctx, value, va);
    va_end(va);
}
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 0)
void ini_parser_value_va(struct IniParserContext *ctx, const char *value, va_list va) {
    assert(ctx->field >= 0);

    struct IniParserSection *s = __ini_parser_current_section(ctx);
    struct IniParserField   *f = s->ptr + ctx->field;

    ctx->tmp.len = 1;
    unsigned int toff = push_fmt_va(&ctx->allocator, &ctx->tmp, value, va);

    const char *tmp = ctx->tmp.ptr + toff;
    f->value_raw = ctx->str.len;

    // check if there are any escapable characters in string
    bool has_escapable = strpbrk(tmp, INIPARSE_ESCAPE_CH) != NULL;

    if(has_escapable) {
        f->has_escapable_ch = true;
        f->value_raw        = ctx->str.len;

        pushc(&ctx->allocator, &ctx->str, '"');
        const char *substr = tmp;
        while(*substr) {
            const char *endptr = strpbrk(substr, INIPARSE_ESCAPE_CH);
            if(!endptr) {
                push(&ctx->allocator, &ctx->str, substr);
                // delete null terminator
                ctx->str.len--;
                break;
            }

            size_t len = endptr - substr;
            push(&ctx->allocator, &ctx->str, substr, &len);
            // delete null terminator
            ctx->str.len--;

            pushc(&ctx->allocator, &ctx->str, '\\');
            switch(*endptr) {
                case '\\':
                case '"':
                    pushc(&ctx->allocator, &ctx->str, *endptr);
                    break;
                case '\n':
                    pushc(&ctx->allocator, &ctx->str, 'n');
                    break;
            }
            substr = endptr + 1;
        }
        pushc(&ctx->allocator, &ctx->str, '"');
        pushc(&ctx->allocator, &ctx->str, 0);
    } else {
        f->value_raw = push(&ctx->allocator, &ctx->str, tmp);
    }

    // parse value
    const char *str = ctx->str.ptr + f->value_raw;

    f->type = INI_PARSER_TYPE_STRING;

    if(strcmp(str, "null") == 0) {
        f->type = INI_PARSER_TYPE_NULL;
        return;
    }

    if(strcmp(str, "true") == 0) {
        f->type   = INI_PARSER_TYPE_BOOL;
        f->t_bool = true;
        return;
    }

    if(strcmp(str, "false") == 0) {
        f->type   = INI_PARSER_TYPE_BOOL;
        f->t_bool = false;
        return;
    }

    size_t str_len = strlen(str);

    char *endptr = NULL;
    long l = strtol(str, &endptr, 10);
    if((str + str_len) == endptr) {
        f->type      = INI_PARSER_TYPE_INTEGER;
        f->t_integer = l;
        return;
    }

    l = strtol(str, &endptr, 16);
    if((str + str_len) == endptr) {
        f->type      = INI_PARSER_TYPE_INTEGER;
        f->t_integer = l;
        return;
    }

    double d = strtod(str, &endptr);
    if((str + str_len) == endptr) {
        f->type    = INI_PARSER_TYPE_FLOAT;
        f->t_float = d;
    }
}

INIPARSE_INLINE INIPARSE_FMTFUNC(2, 3)
void ini_parser_comment(struct IniParserContext *ctx, const char *comment, ...) {
    va_list va;
    va_start(va, comment);
    ini_parser_comment_va(ctx, comment, va);
    va_end(va);
}
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 0)
void ini_parser_comment_va(struct IniParserContext *ctx, const char *comment, va_list va) {
    struct IniParserSection *s = __ini_parser_current_section(ctx);

    if(ctx->field >= 0) {
        struct IniParserField *f = s->ptr + ctx->field;
        f->comment = push_fmt_va(&ctx->allocator, &ctx->str, comment, va);
    } else {
        s->comment = push_fmt_va(&ctx->allocator, &ctx->str, comment, va);
    }
}

INIPARSE_INLINE INIPARSE_FMTFUNC(2, 3)
bool ini_parser_exists(struct IniParserContext *ctx, const char *name, ...) {
    va_list va;
    va_start(va, name);
    bool result = ini_parser_exists_va(ctx, name, va);
    va_end(va);

    return result;
}
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 0)
bool ini_parser_exists_va(struct IniParserContext *ctx, const char *name, va_list va) {
    struct IniParserSection *s = __ini_parser_current_section(ctx);

    ctx->tmp.len = 1;
    unsigned int off  = name ? push_fmt_va(&ctx->allocator, &ctx->tmp, name, va) : 0;
    const char *fname = ctx->tmp.ptr + off;

    if(!fname) {
        return false;
    }

    for(unsigned int i = 0; i < s->len; ++i) {
        const char *cname = ctx->str.ptr + s->ptr[i].name;

        if(strcmp(cname, fname) == 0) {
            return true;
        }
    }

    return false;
}

INIPARSE_INLINE INIPARSE_FMTFUNC(2, 3)
const char *ini_parser_read_string(struct IniParserContext *ctx, const char *name, ...) {
    va_list va;
    va_start(va, name);
    const char *result = ini_parser_read_string_va(ctx, name, va);
    va_end(va);

    return result;
}
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 0)
const char *ini_parser_read_string_va(struct IniParserContext *ctx, const char *name, va_list va) {
    struct IniParserSection *s = __ini_parser_current_section(ctx);

    ctx->tmp.len = 1;
    unsigned int off  = name ? push_fmt_va(&ctx->allocator, &ctx->tmp, name, va) : 0;
    const char *fname = ctx->tmp.ptr + off;

    if(!fname) {
        return "";
    }

    for(unsigned int i = 0; i < s->len; ++i) {
        struct IniParserField *f = s->ptr + i;
        const char *cname = ctx->str.ptr + f->name;

        if(strcmp(cname, fname) == 0) {
            switch(f->type) {
                case INI_PARSER_TYPE_NULL:    break;

                case INI_PARSER_TYPE_BOOL:
                case INI_PARSER_TYPE_INTEGER:
                case INI_PARSER_TYPE_FLOAT:   
                case INI_PARSER_TYPE_STRING:  return ctx->str.ptr + f->value_raw;
            }

            return "";
        }
    }

    return "";
}

INIPARSE_INLINE INIPARSE_FMTFUNC(2, 3)
bool ini_parser_read_bool(struct IniParserContext *ctx, const char *name, ...) {
    va_list va;
    va_start(va, name);
    bool result = ini_parser_read_bool_va(ctx, name, va);
    va_end(va);

    return result;
}
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 0)
bool ini_parser_read_bool_va(struct IniParserContext *ctx, const char *name, va_list va) {
    struct IniParserSection *s = __ini_parser_current_section(ctx);

    ctx->tmp.len = 1;
    unsigned int off  = name ? push_fmt_va(&ctx->allocator, &ctx->tmp, name, va) : 0;
    const char *fname = ctx->tmp.ptr + off;


    if(!fname) {
        return false;
    }

    for(unsigned int i = 0; i < s->len; ++i) {
        struct IniParserField *f = s->ptr + i;
        const char *cname = ctx->str.ptr + f->name;

        if(strcmp(cname, fname) == 0) {
            switch(f->type) {
                case INI_PARSER_TYPE_NULL:    break;

                case INI_PARSER_TYPE_BOOL:
                    return f->t_bool;
                case INI_PARSER_TYPE_INTEGER:
                    return f->t_integer ? true : false;
                case INI_PARSER_TYPE_FLOAT:   
                    return f->t_float ? true : false;

                case INI_PARSER_TYPE_STRING:
                    return strcmp(ctx->str.ptr + f->value_raw, "false") != 0;
            }

            return false;
        }
    }

    return false;
}

INIPARSE_INLINE INIPARSE_FMTFUNC(2, 3)
long ini_parser_read_integer(struct IniParserContext *ctx, const char *name, ...) {
    va_list va;
    va_start(va, name);
    long result = ini_parser_read_integer_va(ctx, name, va);
    va_end(va);

    return result;
}
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 0)
long ini_parser_read_integer_va(struct IniParserContext *ctx, const char *name, va_list va) {
    struct IniParserSection *s = __ini_parser_current_section(ctx);

    ctx->tmp.len = 1;
    unsigned int off  = name ? push_fmt_va(&ctx->allocator, &ctx->tmp, name, va) : 0;
    const char *fname = ctx->tmp.ptr + off;

    if(!fname) {
        return 0;
    }

    for(unsigned int i = 0; i < s->len; ++i) {
        struct IniParserField *f = s->ptr + i;
        const char *cname = ctx->str.ptr + f->name;

        if(strcmp(cname, fname) == 0) {
            switch(f->type) {
                case INI_PARSER_TYPE_NULL:    break;

                case INI_PARSER_TYPE_BOOL:
                    return f->t_bool ? 1 : 0;
                case INI_PARSER_TYPE_INTEGER:
                    return f->t_integer;
                case INI_PARSER_TYPE_FLOAT:   
                    return (long)f->t_float;

                case INI_PARSER_TYPE_STRING:
                    return strlen(ctx->str.ptr + f->value_raw);
            }

            return 0;
        }
    }

    return 0;
}

INIPARSE_INLINE INIPARSE_FMTFUNC(2, 3)
double ini_parser_read_float(struct IniParserContext *ctx, const char *name, ...) {
    va_list va;
    va_start(va, name);
    double result = ini_parser_read_float_va(ctx, name, va);
    va_end(va);

    return result;
}
INIPARSE_INLINE INIPARSE_FMTFUNC(2, 0)
double ini_parser_read_float_va(struct IniParserContext *ctx, const char *name, va_list va) {
    struct IniParserSection *s = __ini_parser_current_section(ctx);

    ctx->tmp.len = 1;
    const char *fname =
        name ? push_fmt_va(&ctx->allocator, &ctx->tmp, name, va) + ctx->tmp.ptr : NULL;

    if(!fname) {
        return 0.0;
    }

    for(unsigned int i = 0; i < s->len; ++i) {
        struct IniParserField *f = s->ptr + i;
        const char *cname = ctx->str.ptr + f->name;

        if(strcmp(cname, fname) == 0) {
            switch(f->type) {
                case INI_PARSER_TYPE_NULL:    break;

                case INI_PARSER_TYPE_BOOL:
                    return f->t_bool ? 1.0 : 0.0;
                case INI_PARSER_TYPE_INTEGER:
                    return (double)f->t_integer;
                case INI_PARSER_TYPE_FLOAT:   
                    return f->t_float;

                case INI_PARSER_TYPE_STRING:
                    return (double)strlen(ctx->str.ptr + f->value_raw);
            }

            return 0.0;
        }
    }

    return 0.0;
}

INIPARSE_INLINE
bool ini_parser_flag_write(struct IniParserContext *ctx, enum IniParserFlag flag, bool value) {
    if(value) {
        ctx->flags |= (1 << (int)flag);
    } else {
        ctx->flags &= ~(1 << (int)flag);
    }

    return value;
}
INIPARSE_INLINE
bool ini_parser_flag_read(struct IniParserContext *ctx, enum IniParserFlag flag) {
    return (ctx->flags & (1 << (unsigned int)flag)) == (1u << (unsigned int)flag);
}

INIPARSE_INLINE
size_t __ini_parser_stream_file(void *target, size_t n, const void *bytes) {
    FILE *f = (FILE *)target;

    return fwrite(bytes, n, 1, f);
}

INIPARSE_INLINE INIPARSE_FMTFUNC(4, 5)
size_t __ini_parser_write_fmt(
    struct IniParserContext *ctx, IniParserStreamFn *stream, void *target,
    const char *fmt, ...
) {
    ctx->tmp.len = 1;
    va_list va;
    va_start(va, fmt);
    unsigned int off = push_fmt_va(&ctx->allocator, &ctx->tmp, fmt, va);
    va_end(va);

    const char *message = ctx->tmp.ptr + off;
    size_t message_len  = ctx->tmp.len - off;
    message_len = message_len ? message_len - 1 : message_len;

    return stream(target, message_len, message);
}

INIPARSE_INLINE
size_t ini_parser_serialize_stream(
    struct IniParserContext *ctx, IniParserStreamFn *stream, void *target
) {
    #define write(fmt, ...) \
        __ini_parser_write_fmt(ctx, stream, target, fmt __VA_OPT__(,) __VA_ARGS__ )

    size_t result = 0;

    for(unsigned int i = 0; i < ctx->len; ++i) {
        struct IniParserSection *s = ctx->ptr + i;

        if(s->comment) {
            const char *comment = ctx->str.ptr + s->comment;

            while(*comment) {
                const char *endptr = strchr(comment, '\n');
                if(endptr) {
                    result += write("# %.*s\n", (int)(endptr - comment), comment);
                    comment += (endptr - comment) + 1;
                } else {
                    result += write("# %s\n", comment);
                    break;
                }
            }
        }

        if(s->name) {
            const char *name = ctx->str.ptr + s->name;

            result += write("[%s]\n", name);
        } else if(i) {
            result += write("[]\n");
        }

        write("\n");

        for(unsigned int j = 0; j < s->len; ++j) {
            struct IniParserField *f = s->ptr + j;

            if(f->comment) {
                const char *comment = ctx->str.ptr + f->comment;

                while(*comment) {
                    const char *endptr = strchr(comment, '\n');
                    if(endptr) {
                        result += write("# %.*s\n", (int)(endptr - comment), comment);
                        comment += (endptr - comment) + 1;
                    } else {
                        result += write("# %s\n", comment);
                        break;
                    }
                }
            }

            const char *name = ctx->str.ptr + f->name;
            const char *v    = ctx->str.ptr + f->value_raw;

            result += write("%s = %s\n", name, v);

            write("\n");
        }
    }

    #undef write
    return result;
}
INIPARSE_INLINE
bool ini_parser_serialize_file_path(
    struct IniParserContext *ctx, const char *fp
) {
    // check if exists
    FILE *f = fopen(fp, "r");
    bool file_exists = f != NULL;

    if(f) {
        fclose(f);
    }

    if(ini_parser_flag_read(ctx, INI_PARSER_FLAG_SERIALIZE_FAIL_IF_EXISTS)) {
        if(file_exists) {
            return false;
        }
    }

    const char *mode;
    if(ini_parser_flag_read(ctx, INI_PARSER_FLAG_SERIALIZE_APPEND_IF_EXISTS)) {
        mode = "ab";
    } else {
        mode = "wb";
    }

    f = fopen(fp, mode);
    if(!f) {
        return false;
    }

    ini_parser_serialize_stream(ctx, __ini_parser_stream_file, f);
    fclose(f);

    return true;
}
INIPARSE_INLINE
void ini_parser_serialize_file(struct IniParserContext *ctx, FILE *f) {
    ini_parser_serialize_stream(ctx, __ini_parser_stream_file, f);
}

INIPARSE_INLINE INIPARSE_FMTFUNC(2, 3)
void __ini_parser_print_error(bool can_print, const char *fmt, ...) {
    if(!can_print) {
        return;
    }

    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
}

INIPARSE_INLINE
bool ini_parser_deserialize(struct IniParserContext *ctx, size_t n, const void *bytes) {
    #define err(fmt, ...) \
        __ini_parser_print_error(ini_parser_flag_read(ctx, INI_PARSER_FLAG_DESERIALIZE_PRINT_ERRORS), fmt __VA_OPT__(,) __VA_ARGS__ )

    // TODO(alicia): preserve comments

    // empty just doesn't add anything to context, correct behaviour
    if(!n) {
        return true;
    }

    bool allocated_buffer = false;

    const char *str = (const char *)bytes;
    // check for null terminator
    if(str[n - 1]) {
        // no null terminator, allocate a new buffer with null at the end
        char *buffer = (char *)__ini_parser_alloc(&ctx->allocator, n + 1);
        allocated_buffer = true;

        memcpy(buffer, bytes, n);
        buffer[n] = 0;

        str = (const char *)buffer;
    }

    struct IniParserStringBuffer linebuf;
    memset(&linebuf, 0, sizeof(linebuf));

    bool success = true;

    int ln = 0;
    const char *substr = str;
    while(*substr && (substr < (str + n))) {
        const char *endline = strchr(substr, '\n');
        if(!endline) {
            endline = substr + strlen(substr);
        }
        ln++;

        reserve(&ctx->allocator, &linebuf, (endline - substr) + 1);
        linebuf.len = (endline - substr);
        memcpy(linebuf.ptr, substr, endline - substr);
        linebuf.ptr[linebuf.len++] = 0;

        char *beginl      = linebuf.ptr;
        char *endl = beginl + (linebuf.len - 1);
        const char *eq  = NULL;
        int field_name_len = 0;
        // eliminate leading whitespace
        while(*beginl) {
            switch(*beginl) {
                case ' ':
                case '\t':
                case '\r':
                    beginl++;
                    continue;
            }
            break;
        }

        // check if comment
        if(*beginl == '#') {
            goto skip_line;
        }

        // eliminate trailing whitespace
        do {
            switch(*endl) {
                case ' ':
                case '\t':
                case '\r':
                    *endl = 0;
                    endl--;
                    continue;
            }
            break;
        } while(endl != beginl);

        if(endl == beginl) {
            goto skip_line;
        }
        // adjust to be the last character before null
        endl--;

        // we have a section!
        if((*beginl == '[') && (*endl == ']')) {
            // terminate previous section if open
            if(ctx->section >= 0) {
                ini_parser_end_section(ctx);
            }
            // open new section with this section name
            int len = (endl - beginl) - 1; // remove []
            ini_parser_begin_section(ctx, "%.*s", len, beginl + 1);
            // end parsing this line
            goto skip_line;
        }

        // we potentially have a field
        eq = strchr(beginl, '=');

        if(!eq) {
            // failed to parse!
            success = false;
            err("%d: invalid syntax", ln);
            goto end;
        }

        field_name_len = eq - beginl;
        // eliminate trailing whitespace from field name
        while(field_name_len) {
            switch(beginl[field_name_len - 1]) {
                case ' ':
                case '\t':
                case '\r':
                    field_name_len--;
                    continue;
            }
            break;
        }

        // begin field
        ini_parser_begin_field(ctx, "%.*s", field_name_len, beginl);
        beginl = (char *)(eq + 1);

        // eliminate leading whitespace
        while(*beginl) {
            switch(*beginl) {
                case ' ':
                case '\t':
                case '\r':
                    beginl++;
                    continue;
            }
            break;
        }

        // if not end of line
        if(*beginl) {
            // trailing whitespace has already been eliminated

            // check for surrounding quotes
            if((*beginl == *endl) && (*beginl == '"')) {
                beginl++;
                *endl = 0;
                // adjust to be last character before null
                endl--;
            }

            // add value
            ini_parser_value(ctx, "%s", beginl);
        }

        // end field
        ini_parser_end_field(ctx);

skip_line:
        substr = endline + 1;
    }

end:
    if(linebuf.ptr) {
        __ini_parser_free(&ctx->allocator, linebuf.ptr, linebuf.cap);
    }
    if(allocated_buffer) {
        __ini_parser_free(&ctx->allocator, (void *)str, n + 1);
    }

    #undef err
    return success;
}
INIPARSE_INLINE
bool ini_parser_deserialize_file_path(struct IniParserContext *ctx, const char *fp) {
    FILE *f = fopen(fp, "rb");
    if(!f) {
        return false;
    }

    bool result = ini_parser_deserialize_file(ctx, f);

    fclose(f);
    return result;
}

INIPARSE_INLINE
bool ini_parser_deserialize_file(struct IniParserContext *ctx, FILE *f) {
    fseek(f, 0, SEEK_END);
    int fsz = ftell(f);
    int n   = fsz + 1;
    fseek(f, 0, SEEK_SET);

    char *contents = (char *)__ini_parser_alloc(&ctx->allocator, n);
    fread(contents, fsz, 1, f);
    contents[fsz] = 0;

    bool result = ini_parser_deserialize(ctx, n, contents);

    __ini_parser_free(&ctx->allocator, contents, n);

    return result;
}

#undef INIPARSE_FMTFUNC
#undef INIPARSE_INLINE
#undef INIPARSE_ESCAPE_CH
#undef STRUCT
#undef reserve
#undef pushc
#undef push
#undef pushn
#undef push_fmt_va
#undef push_fmt
#endif /* header guard */
