#!/bin/sh

# This script sets up the build.
libtoolize --install
aclocal
automake --add-missing
autoreconf --force --install -I config -I m4
autoconf
# Done setting things up.
echo "You may now run the configure script."

