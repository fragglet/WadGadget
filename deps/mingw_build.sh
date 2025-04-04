#!/usr/bin/env bash
#
# Build script for Mingw (Windows) compile

set -eu

export HOST=x86_64-w64-mingw32
export CC=$HOST-gcc
export LDSHAREDLIBC=
export INST=$PWD/inst
export PKG_CONFIG_PATH=$INST/lib/pkgconfig:${PKG_CONFIG_PATH:-}
export CFLAGS=-I$INST/include
export CPPFLAGS=-I$INST/include
export LDFLAGS=-L$INST/lib

pushd pdcurses/wincon
make CC=$HOST-gcc \
     AR=$HOST-ar \
     STRIP=$HOST-strip \
     LINK=$HOST-gcc \
     WINDRES=$HOST-windres
mkdir -p $INST/include $INST/lib
cp ../curses.h $INST/include
cp pdcurses.a $INST/lib
popd

pushd zlib
./configure --prefix=$INST --static
make
make install
popd

pushd libpng
./configure --prefix=$INST \
            --disable-shared \
            --disable-tests \
            --disable-tools
make
make install
popd

autogen --version  # Needs to be installed
pushd libsndfile
autoreconf -fi
./configure --prefix=$INST \
            --host=$HOST \
            --disable-shared \
            --disable-full-suite
make
make install
popd
