#!/bin/sh

PROJECT="mvt"

test -n "$srcdir" || srcdir="`dirname \"$0\"`"
test -n "$srcdir" || srcdir=.

if ! test -f "$srcdir/configure.ac"; then
    echo "Failed to find the top-level $PROJECT directory"
    exit 1
fi

olddir="`pwd`"
cd "$srcdir"

mkdir -p m4

GIT=`which git`
if test -z "$GIT"; then
    echo "*** No git found ***"
    exit 1
else
    submodule_init="no"
    [ -f ext/libyuv/upstream/include/libyuv.h ] || submodule_init="yes"
    [ -f ext/ffmpeg/upstream/configure ] || submodule_init="yes"
    [ -f ext/libvpx/upstream/configure ] || submodule_init="yes"
    if test "$submodule_init" = "yes"; then
        $GIT submodule init
    fi
    $GIT submodule update
fi

AUTORECONF=`which autoreconf`
if test -z "$AUTORECONF"; then
    echo "*** No autoreconf found ***"
    exit 1
else
    autoreconf -v --install || exit $?
fi

cd "$olddir"

if test -z "$NOCONFIGURE"; then
    $srcdir/configure "$@" && echo "Now type 'make' to compile $PROJECT."
fi
