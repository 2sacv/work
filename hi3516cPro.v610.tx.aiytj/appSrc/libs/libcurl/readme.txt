libcurl 依赖 openssl ,请先编译openssl

## libcurl编译：

下载：

[https://curl.se/libcurl/](https://curl.se/libcurl/)

指令：

./configure --prefix=${PWD}/_install --disable-shared --enable-static  --with-ssl=../../openssl/  --without-libidn --without-librtmp  --without-nss --without-libssh2 --without-zlib --without-winidn --without-gnutls --disable-rtsp  --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smtp --disable-gopher --disable-ldap --host=arm-v01c02-linux-musleabi  CC=arm-v01c02-linux-musleabi-gcc

make&&make install


https支持单独编译注意事项，当前暂不支持https
--with-ssl=../../openssl/openssl-OpenSSL_1_1_1-stable/_install
并且修改lib\vtls\openssl.c  注释掉case EVP_PKEY_DH: 整个case，否则会编译不通过，提示找不到DH相关的函数
注意，辅助libcurl编译openssl 需要编译动态库  enable-shared
CFLAGS="-O2" CPPFLAGS="-O2" CXXFLAGS="-O2" LDFLAGS="-O2" ./Configure  linux-armv4 no-asm enable-shared no-async no-idea no-camellia no-seed no-bf no-cast no-rc2 no-rc4 no-rc5 no-md2 no-md4 no-mdc2 no-dsa no-dh no-ec no-ecdsa no-ec no-ecdsa no-ecdh no-err no-engine no-hw --cross-compile-prefix=arm-v01c02-linux-musleabi- --prefix=${PWD}/_install
