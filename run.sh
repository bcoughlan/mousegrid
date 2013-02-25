#!/bin/bash
gcc -Wall -g mousegrid.c -o mousething `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0` -std=c99; ./mousething

