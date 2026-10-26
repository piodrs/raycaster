#!/bin/sh

gcc -Wall -Wextra -std=c11 -pedantic -ggdb $(pkg-config --cflags raylib) -o rc main.c $(pkg-config --libs raylib) -lm
