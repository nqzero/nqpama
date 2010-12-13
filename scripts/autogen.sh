#!/bin/bash

libtoolize
aclocal
autoheader
automake --add-missing
autoconf
echo now call ./configure, see README for more details



