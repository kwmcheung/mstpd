#!/bin/sh

[ -e .gitmodules ] && [ -e .git ] && {
    echo "autogen.sh: updating git submodules"
    git submodule init
    git submodule update
}

autoreconf -W portability -visf
