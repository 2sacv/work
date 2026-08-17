#! /bin/sh
#---------------------------------------------------------------------------
#          FILE: build.sh
#         USAGE: ./build.sh 
#   DESCRIPTION: 
#       OPTIONS: -
#  REQUIREMENTS: -
#          BUGS: -
#         NOTES: -
#        AUTHOR: xiangyp () 
#  ORGANIZATION: 
#       CREATED: 2026-07-16 09:49:16 PM
#      REVISION: 1.0 
#---------------------------------------------------------------------------

fn_main()
{
    CFLAGS="-O2" CPPFLAGS="-O2" CXXFLAGS="-O2" LDFLAGS="-O2" \
    ./Configure no-asm shared no-async linux-armv4 no-asm no-shared no-async no-idea no-bf no-cast no-rc2 no-rc4 no-rc5 no-md2 no-md4 no-mdc2 no-ec no-hw \
        --prefix=${PWD}/.install \
        --cross-compile-prefix=arm-v01c02-linux-musleabi-

    make -j$(nproc)
    make install
}

fn_main $@

