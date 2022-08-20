#!/bin/sh

# generate compilation database for Clang tools.
make clean
bear -- make all
