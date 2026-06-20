# Ini Parser

Simple header-only library for parsing .ini files.

## Usage

```c
// API is entirely header-only, no need to define implementation
#include "ini-parser.h"

int main(int argc, char **argv) {

    // create a context
    struct IniParserContext ctx;

    // begin/end the context
    ini_parser_begin(&ctx); {
        // define a section (optional)
        ini_parser_begin_section(&ctx, "some-section"); {
            // add a comment to section (optional)
            ini_parser_comment(&ctx, "some comment for section");

            // define a field
            ini_parser_begin_field(&ctx, "some-field"); {
                // add a comment to field (optional)
                ini_parser_comment(&ctx, "some comment for field");

                // define the field's value, in this case a string
                ini_parser_value(&ctx, "some value");

                // end field
                ini_parser_end_field(&ctx);
            }

            // end the section.
            ini_parser_end_section(&ctx);
        }

        // deserialize existing file.
        // this will append new sections/fields and/or modify existing ones
        // and replace existing field values from file
        ini_parser_deserialize_file_path(&ctx, "some-file.ini");

        // read field (section-less)
        long i = ini_parser_read_integer(&ctx, "some-integer");

        // read field from section
        ini_parser_begin_section(&ctx, "some-section"); {
            // read field as a string
            // in this case 'some value'
            // unless it was overwritten by deserialized value
            const char *str = ini_parser_read_string(&ctx, "some-field");
            ini_parser_end_section(&ctx);
        }

        // serialize to file, in this case stdout
        ini_parser_serialize_file(&ctx, stdout);
        
        // do not call any more ini_parser functions after this
        ini_parser_end(&ctx);
    }

    return 0;
}
```

