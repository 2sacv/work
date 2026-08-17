Version: openssl-OpenSSL_1_1_1

Machine: arm gnu

openssl-OpenSSL_1_1_1-stable.zip

使用以下配置生成makefile，为了减少库的大小，下面配置是进行了一些裁剪，
//CFLAGS="-O2" CPPFLAGS="-O2" CXXFLAGS="-O2" LDFLAGS="-O2" ./Configure  linux-armv4 no-asm no-shared no-async no-idea no-camellia no-seed no-bf no-cast no-rc2 no-rc4 no-rc5 no-md2 no-md4 no-mdc2 no-dsa no-dh no-ec no-ecdsa no-ec no-ecdsa no-ecdh no-err no-engine no-hw --cross-compile-prefix=arm-v01c02-linux-musleabi- --prefix=${PWD}/_install

CFLAGS="-O2" CPPFLAGS="-O2" CXXFLAGS="-O2" LDFLAGS="-O2" ./Configure  linux-armv4 no-asm no-shared no-async no-idea no-bf no-cast no-rc2 no-rc4 no-rc5 no-md2 no-md4 no-mdc2 no-ec no-ecdsa no-ecdsa no-hw --cross-compile-prefix=arm-v01c02-linux-musleabi- --prefix=${PWD}/__install
make

make install

* no-asm: 在交叉编译过程中不使用汇编代码代码加速编译过程。原因是它的汇编代码是对arm格式不支持的。
* shared: 生成动态连接库。
* no-async: 交叉编译工具链没有提供GNU C的ucontext库，否则会报错`undefined reference to getcontext`
* –prefix=: 安装路径，编译完成install后将有bin，lib，include等文件夹
* COMPILE=: 交叉编译工具，在 Makefile 中直接改，不要用 –cross-compile-prefix 指定
