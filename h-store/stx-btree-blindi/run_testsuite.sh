#!/usr/bin/env bash

aclocal
automake --add-missing
autoconf
configure
make clean
make all
make check