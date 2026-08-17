# zlib 编译教程

使用以下配置生成库。

```bash
    CROSS_COMPILE=arm-v01c02-linux-musleabi-

    CC=${CROSS_COMPILE}gcc \
    AR=${CROSS_COMPILE}ar \
    RANLIB=${CROSS_COMPILE}ranlib \
    ./configure --prefix=${PWD}/.install

    make -j$(nproc)
    make install
```

以上内容已同步更新到 build.sh，解压后将 build.sh 复制到解压后的 `zlib-1.2.13`，操作如下所示：

```bash
cp build.sh zlib-1.2.13/
cd zlib-1.2.13/
./build.sh
cp -rf .install/include ../
cp -rf .install/lib/*.a ../
```
