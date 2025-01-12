#!/usr/bin/env bash
gcc `pkg-config --cflags --libs notcurses-core` -Wall -o unn src/unn.c