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
#       CREATED: 2026-07-17 09:03:01 AM
#      REVISION: 1.0 
#---------------------------------------------------------------------------

fn_main()
{
    CROSS_COMPILE=arm-v01c02-linux-musleabi-

    CC=${CROSS_COMPILE}gcc \
    AR=${CROSS_COMPILE}ar \
    RANLIB=${CROSS_COMPILE}ranlib \
    ./configure --prefix=${PWD}/.install

    make -j$(nproc)
    make install
}

fn_main $@

