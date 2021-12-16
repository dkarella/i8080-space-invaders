#!/bin/bash

clang -std=c99 -Wall -Werror -o main main.c && ./main $@
