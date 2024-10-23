#!/bin/bash

find . -name Makefile | xargs sed -i 's/[ \t]*$//'

#find . -type f -name "*.c" -o -name "*.h" | xargs astyle --style=linux -n

find . -iname *.h -o -iname *.c | xargs clang-format -i -style=file

exit 0