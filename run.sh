#!/usr/bin/env bash

if [ ! -f "./example" ] || [ example.c -nt ./example ] || [ ini-parser.h -nt ./example ]; then
    set -x
    gcc example.c -o example -ggdb -O0 -Wall -Wextra
    { set +x; } 2>/dev/null
fi

./example "$@"

