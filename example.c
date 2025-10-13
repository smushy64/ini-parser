/**
 * @file   example.c
 * @brief  Example.
 * @author Alicia Amarilla (smushyaa@gmail.com)
 * @date   October 11, 2025
*/
#include "ini-parse.h"

int main( int argc, char** argv ) {
    const char* path = "example.ini";
    if( argc >= 2 ) {
        path = argv[1];
    }

    IniCtx ctx = {0};

    ini_open( path, &ctx );

    ini_set_section_comment( &ctx, NULL, "this is an example file for ini-parse.h" );
    ini_set_section_comment( &ctx, "section", "section names cannot include [ or ]" );

    ini_read( &ctx, NULL, "bar", .default_value="baz", .comment="example of a key/value pair" );
    ini_read( &ctx, NULL, "foo", .default_value="bar", .comment="another example of a key/value pair" );

    ini_read(
        &ctx, "section", "string-quotes", .default_value="\"  Quoted string.\"",
        .comment="strings can be quoted in order to include surrounding whitespace" );

    ini_serialize_to_file( &ctx, path, true );

    ini_close( &ctx );
    return 0;
}

