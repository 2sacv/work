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
#       CREATED: 2026-07-16 09:06:18 PM
#      REVISION: 1.0 
#---------------------------------------------------------------------------

fn_main()
{
    CROSS_COMPILE=/opt/linux/x86-arm/arm-v01c02-linux-musleabi-gcc/bin/arm-v01c02-linux-musleabi-
    CC=${CROSS_COMPILE}gcc
    CXX=${CROSS_COMPILE}g++
    AR=${CROSS_COMPILE}ar
    RANLIB=${CROSS_COMPILE}ranlib

    base_dir=${PWD}/../../../

    cmake ..                                                                                    \
        -DCMAKE_SYSTEM_NAME=Linux                                                               \
        -DCMAKE_C_COMPILER=${CC}                                                                \
        -DCMAKE_CXX_COMPILER=${CXX}                                                             \
        -DCMAKE_AR=${AR}                                                                        \
        -DCMAKE_RANLIB=${RANLIB}                                                                \
        -DCMAKE_INSTALL_PREFIX=${PWD}/../.install                                               \
        -DCMAKE_C_FLAGS="-ffunction-sections -fdata-sections -flto"                             \
        -DCMAKE_EXE_LINKER_FLAGS="-Wl,--gc-sections -flto"                                      \
        -DCMAKE_SHARED_LINKER_FLAGS="-Wl,--gc-sections -flto"                                   \
        -DCMAKE_BUILD_TYPE=MinSizeRel                                                           \
        -DLWS_WITH_SSL=ON                                                                       \
        -DLWS_OPENSSL_INCLUDE_DIRS=${base_dir}/openssl/include                                  \
        -DLWS_OPENSSL_LIBRARIES="${base_dir}/openssl/libssl.a;${base_dir}/openssl/libcrypto.a"  \
        -DLWS_WITH_ZLIB=ON                                                                      \
        -DZLIB_INCLUDE_DIR=${base_dir}/zlib/include                                             \
        -DZLIB_LIBRARY=${base_dir}/zlib/libz.a                                                  \
        -DLWS_WITHOUT_SERVER=ON                                                                 \
        -DLWS_WITHOUT_TESTAPPS=ON                                                               \
        -DLWS_WITH_SHARED=OFF                                                                   \
        -DLWS_WITH_STATIC=ON                                                                    \
        -DLWS_WITHOUT_DEBUG=ON                                                                  \
        -DLWS_WITHOUT_DAEMONIZE=ON                                                              \
        -DLWS_WITHOUT_BUILTIN_GETIFADDRS=ON                                                     \
        -DLWS_WITH_HTTP2=OFF                                                                    \
        -DLWS_WITH_HTTP_BASIC_AUTH=ON                                                           \
        -DLWS_WITH_HTTP_STREAM_COMPRESSION=OFF                                                  \
        -DLWS_WITH_CGI=OFF                                                                      \
        -DLWS_WITH_ACCESS_LOG=OFF                                                               \
        -DLWS_WITH_RANGES=ON                                                                    \
        -DLWS_WITH_SEQUENCER=ON                                                                 \
        -DLWS_WITH_SOCKS5=OFF                                                                   \
        -DLWS_WITH_UNIX_SOCK=OFF                                                                \
        -DLWS_WITH_PEER_LIMITS=OFF                                                              \
        -DLWS_WITH_PLUGINS=OFF                                                                  \
        -DLWS_WITH_HTTP_PROXY=OFF                                                               \
        -DLWS_WITH_ACME=OFF                                                                     \
        -DLWS_WITH_JWS=OFF                                                                      \
        -DLWS_WITH_JOSE=OFF                                                                     \
        -DLWS_WITH_SECURE_STREAMS=ON                                                            \
        -DLWS_WITH_SECURE_STREAMS_PROXY_API=OFF                                                 \
        -DLWS_SS_USE_STATIC_POLICY=OFF                                                          \
        -DLWS_WITH_SECURE_STREAMS_CPP=OFF                                                       \
        -DLWS_WITH_SS_DIRECT_PROTOCOL_OPS=OFF                                                   \
        -DLWS_WITH_LEJP=ON                                                                      \
        -DLWS_WITH_LEJP_CONF=OFF                                                                \
        -DLWS_WITH_TLS_SESSIONS=ON                                                              \
        -DLWS_WITHOUT_EXTENSIONS=ON                                                             \
        -DLWS_WITH_LIBUV=OFF                                                                    \
        -DLWS_WITH_LIBEV=OFF                                                                    \
        -DLWS_WITH_LIBEVENT=OFF                                                                 \
        -DLWS_MAX_SMP=1

    make -j$(nproc)
    make install
}

fn_main $@

