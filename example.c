/**
 * @file   example.c
 * @brief  Example of ini parser.
 * @author Alicia D. Amarilla (smushyaa@gmail.com)
 * @date   June 18, 2026
*/
// all functions are inline
#include "ini-parser.h"

int main(int argc, char **argv) {
    (void)argc, (void)argv;
    struct IniParserContext ctx;

    ini_parser_begin(&ctx); {
        ini_parser_begin_section(&ctx, "some_section"); {
            ini_parser_comment(&ctx, "this is a section");

            ini_parser_begin_field(&ctx, "some_null"); {
                ini_parser_value(&ctx, "null");
                ini_parser_comment(&ctx, "this is a null value");
                ini_parser_end_field(&ctx);
            }

            ini_parser_begin_field(&ctx, "some_bool"); {
                ini_parser_value(&ctx, "true");
                ini_parser_end_field(&ctx);
            }

            ini_parser_begin_field(&ctx, "some_integer"); {
                ini_parser_value(&ctx, "34534");
                ini_parser_end_field(&ctx);
            }

            ini_parser_begin_field(&ctx, "some_float"); {
                ini_parser_value(&ctx, "34534.325");
                ini_parser_end_field(&ctx);
            }

            ini_parser_begin_field(&ctx, "some_string"); {
                ini_parser_value(&ctx, "hello, world!");
                ini_parser_end_field(&ctx);
            }

            ini_parser_begin_field(&ctx, "some_other_string"); {
                ini_parser_value(&ctx, "hello, world!\n");
                ini_parser_end_field(&ctx);
            }

            ini_parser_begin_field(&ctx, "some_other_other_string"); {
                ini_parser_value(&ctx, "hello, world!\\\"");
                ini_parser_end_field(&ctx);
            }

            ini_parser_end_section(&ctx);
        }

        ini_parser_serialize_file(&ctx, stdout);

        ini_parser_end(&ctx);
    }

    if(argc > 1) {
        printf("\n\n------ deserialize -----\n\n");
        ini_parser_begin(&ctx);

        if(ini_parser_deserialize_file_path(&ctx, argv[1])) {
            ini_parser_serialize_file(&ctx, stdout);
        } else {
            fprintf(stderr, "failed to open %s!\n", argv[1]);
        }

        ini_parser_end(&ctx);
    }

    return 0;
}

