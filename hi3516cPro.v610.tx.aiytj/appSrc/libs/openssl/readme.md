# openssl 编译教程

使用以下配置生成库，为了减少库的大小，下面配置是进行了一些裁剪。

```bash
    CFLAGS="-O2" CPPFLAGS="-O2" CXXFLAGS="-O2" LDFLAGS="-O2" \
    ./Configure no-asm shared no-async linux-armv4 no-asm no-shared no-async no-idea no-bf no-cast no-rc2 no-rc4 no-rc5 no-md2 no-md4 no-mdc2 no-ec no-hw \ #no-ecdsa
        --prefix=${PWD}/.install \
        --cross-compile-prefix=arm-v01c02-linux-musleabi-

    make -j$(nproc)
    make install
```

| 参数       | 说明                                                                                   |
| ------     | ------                                                                                 |
| `no-asm`   | 在交叉编译过程中不使用汇编代码代码加速编译过程。原因是它的汇编代码是对arm格式不支持的  |
| `shared`   | 生成动态连接库                                                                         |
| `no-async` | 交叉编译工具链没有提供GNU C的ucontext库，否则会报错`undefined reference to getcontext` |
| `–prefix=` | 安装路径，编译完成install后将有bin，lib，include等文件夹                               |
| `COMPILE=` | 交叉编译工具，在 Makefile 中直接改，不要用 –cross-compile-prefix 指定                  |

需要注意的是，如果要用到 websockets 库，则需编入 ecdsa 模块，要去除 no-ecdsa 编译选项。

以上内容已同步更新到 build.sh，解压后将 build.sh 复制到解压后的 `openssl-OpenSSL_1_1_1-stable`，操作如下所示：

```bash
cp build.sh openssl-OpenSSL_1_1_1-stable/
cd openssl-OpenSSL_1_1_1-stable/
./build.sh
cp -rf .install/include ../
cp -rf .install/lib/*.a ../
```
