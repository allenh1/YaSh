#!/bin/sh

# This script sets up the build.
autoreconf --force --install -I config -I m4
libtoolize --install

# Done setting things up.
echo "You may now run the configure script."

