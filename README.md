# Ini Parser

Simple header-only library for parsing .ini files.

## Usage
```c
#include "ini-parse.h"

int main( int argc, char** argv ) {
    // create a zero-initialized context.
    IniCtx ctx;
    memset( &ctx, 0, sizeof(ctx) );

    // open an .ini file
    // if file does not exist,
    // creates an empty context ready for writing to
    ini_open( "example.ini", &ctx );
    // or open from memory
    //
    // char buf[255];
    // ini_open_from_memory( sizeof(buf), buf, &ctx );

    // or create new empty context
    // ini_new( &ctx );

    // read a string
    // use NULL or * to read a section-less key
    const char* data0 = ini_read( &ctx, NULL, "some-data" );

    // read a number
    double data1 = ini_read_number( &ctx, "some-section", "some-number" );

    // read a boolean
    bool data2 = ini_read_number( &ctx, "some-section", "some-boolean" );

    // you can also create key/value pairs by reading missing data
    // and setting the default_value field.
    // if data does not exist and default value field is missing,
    // writes a null instead.
    // ini_read always returns an empty string if data is missing or explicitly null.
    const char* data3 = ini_read( &ctx, "some-section", "missing-data", .default_value="foo" );

    // you can also just create a new key/value pair
    // this function will automatically detect what type the value should be.
    ini_write( &ctx, "section-name", "key-name", "value" );

    // the value parameter is a formatted string so you can do something like this:
    double number = 243.2346236;
    ini_write( &ctx, "section-name", "some-number", "%f", number );

    // setting the section name to NULL adds a key or reads a key from the NULL section
    ini_write( &ctx, NULL, "some-value", "foo" );

    // you can write a comment for a value when reading
    // if the comment field is set, it will overwrite the existing comment or allocate a new comment.
    const char* data4 = ini_read(
        &ctx, "some-section", "some-other-value", .comment="this is a comment" );

    // you can also write a formatted comment like so
    ini_set_comment(
        &ctx, "some-section", "some-other-value", "this is a formatted comment: %d", 10 );

    // or write a comment for a section
    ini_set_comment(
        &ctx, "some-section",
        "this is a comment for the whole section. it is also formatted: %s", "foo" );

    // comments are never parsed from a file so this ensures that
    // necessary comments are written into the file when it's serialized.

    // serialize data back to a file
    // if boolean is set to true, truncates existing file,
    // otherwise it appends to existing file.
    // if file doesn't exist yet, it creates the file.
    ini_serialize_to_file( &ctx, "example.ini", true );

    // free the context
    ini_close( &ctx );
    return 0;
}

```

