#! /bin/sh

LIBPATH='/opt/ingen.cc/gcc-20250305-arm-v01c02-linux-musleabi/arm-v01c02-linux-musleabi-gcc/lib'

arm-linux-musleabi-gdb \
    --init-eval-command="set dir $PWD" \
    --init-eval-command="set solib-search-path  $LIBPATH:$PWD/../filesys/filesys_normal/lib:$PWD/../filesys/filesys_normal/ipc/lib:$PWD/../filesys/filesys_normal/algo:$PWD/sdklibs/lib" \
    -eval-command="info sharedlibrary" \
    $@

