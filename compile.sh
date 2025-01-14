#!/usr/bin/env bash
gcc `pkg-config --cflags --libs notcurses-core` -g -Wall -o unn src/unn.c