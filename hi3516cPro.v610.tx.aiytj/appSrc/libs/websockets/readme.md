# README.md

# libwebsockets v4.5-stable 交叉编译指南

> 适用于 RTOS 嵌入式平台（Hi3516C 系列）
>
> 因 RTOS SDK 中的 OpenSSL 为裁剪版本（缺少 ecdsa.h 等模块），
> 需要单独编译一份完整的 OpenSSL 1.1.1t 给 libwebsockets 使用。
> zlib 使用 RTOS SDK 中已有的版本。

## 版本信息

| 组件          | 版本          | 说明                                                               |
| ------------- | ------------- | ------------------------------------------------------------------ |
| libwebsockets | v4.5-stable   | 本项目编译目标                                                     |
| OpenSSL       | 1.1.1t        | 某些分支为裁剪版，需单独编译包含 ecdsa 模块版（RTOS SDK 的已裁剪） |
| zlib          | zlib-1.2.13   | 复用 RTOS SDK 中的版本                                             |

## 工具链信息

| 项目                   | 值                                       |
| ---------------        | ---------------------------------------- |
| 交叉编译器需要绝对路径 | arm-v01c02-linux-musleabi                |
| 目标平台               | Hi3516C (ARM Cortex-A7)                  |
| CMake 最低版本         | 3.10                                     |

## 目录结构

```bash
.
├── build.sh
├── include
│   ├── libwebsockets
│   ├── libwebsockets.h
│   └── lws_config.h
├── libwebsockets-4.5-stable.zip
├── libwebsockets.a
└── README.md
```

> **zlib** 继续使用 RTOS SDK 中已有的版本，无需单独编译。

## 编译总览

```bash
第一步：单独编译 OpenSSL 1.1.1t（包含 ecdsa 完整版，含 EC 模块）
第二步：编译 libwebsockets v4.5-stable（静态库）
```

---

## 一、编译 OpenSSL 1.1.1t

RTOS SDK 中的 OpenSSL 是裁剪版本，缺少 `ecdsa.h` 等椭圆曲线模块，
libwebsockets 4.5 依赖这些模块，因此必须单独编译一份包含 ecdsa 模块的 OpenSSL，具体可参考 openssl 库里面的 Readme.txt。

### 1.1 编译好之后，验证产出

```bash
# 确认完整的头文件（重点检查 ecdsa.h）
ls ${PWD}/.install/include/openssl/ecdsa.h && echo "OK: ecdsa.h 存在"

# 确认静态库
ls -lh ${PWD}/.install/lib/libssl.a
ls -lh ${PWD}/.install/lib/libcrypto.a

# 确认架构
file ${PWD}/.install/lib/libssl.a

# 确认版本
grep "OPENSSL_VERSION_TEXT" ${PWD}/.install/include/openssl/opensslv.h
```

---

## 二、编译 libwebsockets v4.5-stable

### 2.1 获取源码，已有安装包则跳过此步骤

```bash
cd /opt/ipc/hi3516cPro/hi3516cPro.v610.tx.aiytj/appSrc/libs/websockets
git clone -b v4.5-stable https://github.com/warmcat/libwebsockets.git
cd libwebsockets
mkdir build && cd build
```

### 2.2 确认 zlib 路径

RTOS SDK 中的 zlib 通常未被裁剪，可以直接复用。确认文件存在：

```bash
# 根据实际路径修改
ZLIB_DIR=/opt/ipc/hi3516cPro/hi3516cPro.v610.tx.aiytj/appSrc/libs/zlib/.install

ls ${ZLIB_DIR}/include/zlib.h && echo "OK: zlib.h 存在"
ls ${ZLIB_DIR}/lib/libz.a && echo "OK: libz.a 存在"
```

### 2.3 CMake 配置

```bash
unzip libwebsockets-4.5-stable.zip
cd libwebsockets-4.5-stable/
mkdir build
cp ../build.sh build/
cd build
./build.sh

cd ../
cp -rf .install/include ../
cp -rf .install/lib/*.a ../
```

### 2.4 参数说明

#### 工具链（必须使用绝对路径）

| 参数                 | 说明                                                                        |
| ------               | ------                                                                      |
| `CMAKE_SYSTEM_NAME`  | 告知 CMake 进行交叉编译                                                     |
| `CMAKE_C_COMPILER`   | C 编译器**绝对路径**                                                        |
| `CMAKE_CXX_COMPILER` | C++ 编译器**绝对路径**                                                      |
| `CMAKE_AR`           | ar 归档工具**绝对路径**（必须，否则链接时会报 "No such file or directory"） |
| `CMAKE_RANLIB`       | ranlib 工具**绝对路径**（必须）                                             |

> **为什么必须用绝对路径**：CMake 生成的 Makefile 在子进程 shell 中执行，
> 其 PATH 环境变量可能与当前 shell 不同，使用相对路径或工具名会导致
> "No such file or directory" 错误。

#### OpenSSL（指向单独编译的完整版）

| 参数                       | 说明                                    |
| ------                     | ------                                  |
| `LWS_WITH_SSL`             | 启用 TLS 支持                           |
| `LWS_OPENSSL_INCLUDE_DIRS` | 指向单独编译的 OpenSSL 完整头文件目录   |
| `LWS_OPENSSL_LIBRARIES`    | 指向单独编译的 OpenSSL 静态库，分号分隔 |

> **为什么不能用 RTOS SDK 的 OpenSSL**：SDK 中的 OpenSSL 为裁剪版本，
> 缺少 `ecdsa.h` 等椭圆曲线模块头文件，libwebsockets 4.5 编译时会报
> `unknown type name 'ECDSA_SIG'` 错误。

#### zlib（复用 RTOS SDK）

| 参数                | 说明                             |
| ------              | ------                           |
| `LWS_WITH_ZLIB`     | 启用 permessage-deflate 压缩扩展 |
| `DZLIB_INCLUDE_DIR` | zlib 头文件目录                  |
| `DZLIB_LIBRARY`     | zlib 静态库路径                  |

#### 构建目标（静态库）

| 参数              | 说明                           |
| ------            | ------                         |
| `LWS_WITH_SHARED` | OFF，不生成动态库              |
| `LWS_WITH_STATIC` | ON，生成静态库 libwebsockets.a |

#### 嵌入式精简


| 参数                               | 值                            | 说明                                                                                                                                                                                                       |
| ------                             | ---                           | ------                                                                                                                                                                                                     |
| `CMAKE_BUILD_TYPE`                 | `MinSizeRel`                  | 编译优化策略：优先体积最小化（等价于 GCC `-Os`），牺牲少量性能换取更小的产物体积。可选值：`Debug`（调试信息）、`Release`（性能优先 `-O2`）、`RelWithDebInfo`（性能+调试）、`MinSizeRel`（体积优先）       |
| `LWS_WITH_SSL`                     | `ON`                          | 启用 SSL/TLS 加密支持。开启后 libwebsockets 依赖 OpenSSL（或 mbedTLS），提供 wss:// 加密连接能力。关闭则只能使用 ws:// 明文连接                                                                            |
| `LWS_OPENSSL_INCLUDE_DIRS`         | `${base_dir}/openssl/include` | 指定 OpenSSL 头文件目录（绝对路径）。CMake 会在该目录下查找 `openssl/ssl.h`、`openssl/ecdsa.h` 等头文件。注意：指向的是 `openssl/` 目录的**父目录**，而非 `openssl/` 目录本身                              |
| `LWS_OPENSSL_LIBRARIES`            | `libssl.a;libcrypto.a`        | 指定 OpenSSL 静态库文件路径（绝对路径，多个库用分号分隔）。`libssl.a` 是 TLS 协议实现，`libcrypto.a` 是底层加密算法库。libwebsockets 同时依赖两者                                                          |
| `LWS_WITH_ZLIB`                    | `ON`                          | 启用 zlib 压缩支持，用于 WebSocket 的 `permessage-deflate` 扩展。开启后 WebSocket 消息可以在传输时压缩，减少带宽占用，代价是增加 CPU 开销和少量内存                                                        |
| `DZLIB_INCLUDE_DIR`                | `${base_dir}/zlib/include`    | 指定 zlib 头文件目录（绝对路径）。CMake 会在该目录下查找 `zlib.h`                                                                                                                                          |
| `DZLIB_LIBRARY`                    | `${base_dir}/zlib/libz.a`     | 指定 zlib 静态库文件路径（绝对路径）                                                                                                                                                                       |
| `LWS_WITHOUT_SERVER`               | `ON`                          | **关闭服务端功能**。开启后 libwebsockets 不再包含服务端监听、连接接受、HTTP 响应等代码。如果你的应用只需要作为 WebSocket 客户端连接远程服务器，关闭此项可以大幅减小体积（约 30%~40%）                      |
| `LWS_WITHOUT_TESTAPPS`             | `ON`                          | 不编译 libwebsockets 自带的测试程序和示例程序。减少编译时间和产物体积，对最终库的功能无影响                                                                                                                |
| `LWS_WITH_SHARED`                  | `OFF`                         | 不生成动态共享库（.so）。嵌入式场景通常使用静态链接，避免运行时动态库加载问题                                                                                                                              |
| `LWS_WITH_STATIC`                  | `ON`                          | 生成静态库（.a）。最终产出 `libwebsockets.a`，链接到你的应用中，不依赖运行时的 .so 文件                                                                                                                    |
| `LWS_WITHOUT_DEBUG`                | `ON`                          | 禁用所有调试日志输出代码。关闭后 `lwsl_debug()`、`lwsl_hexdump()` 等调试日志调用会被编译为空操作。减少约 10%~15% 的代码体积，同时运行时无调试日志开销。生产环境建议开启                                    |
| `LWS_WITHOUT_DAEMONIZE`            | `ON`                          | 禁用守护进程化功能（daemon 模式）。嵌入式 RTOS 通常由系统管理进程生命周期，不需要进程自行 fork 到后台运行                                                                                                  |
| `LWS_WITHOUT_BUILTIN_GETIFADDRS`   | `ON`                          | 不使用 libwebsockets 内置的 `getifaddrs` 实现，而是使用系统 C 库提供的版本。如果 RTOS 的 musl libc 已经实现了 `getifaddrs`，开启此项可以去掉重复代码                                                       |
| `LWS_WITH_HTTP2`                   | `OFF`                         | 禁用 HTTP/2 协议支持。HTTP/2 是 HTTP/1.1 的升级版，支持多路复用、头部压缩等。WebSocket 不依赖 HTTP/2，如果不需要 HTTP/2 代理或多路复用，关闭此项可以减小体积和依赖                                         |
| `LWS_WITH_HTTP_BASIC_AUTH`         | `OFF`                         | 禁用 HTTP Basic 认证支持。如果不需要服务端对客户端进行 HTTP 层面的用户名/密码认证，可以关闭。注意：这不影响 WebSocket 层面的自定义鉴权                                                                     |
| `LWS_WITH_HTTP_STREAM_COMPRESSION` | `OFF`                         | 禁用 HTTP 流式压缩。这是 HTTP 层面的 gzip/deflate 压缩（非 WebSocket 的 permessage-deflate），用于 HTTP 响应压缩。客户端场景通常不需要                                                                     |
| `LWS_WITH_CGI`                     | `OFF`                         | 禁用 CGI（Common Gateway Interface）支持。CGI 用于服务端调用外部程序处理 HTTP 请求，嵌入式场景几乎不会用到                                                                                                 |
| `LWS_WITH_ACCESS_LOG`              | `OFF`                         | 禁用 HTTP 访问日志功能。服务端记录每个请求的来源、时间、路径等信息，关闭此项减小体积                                                                                                                       |
| `LWS_WITH_RANGES`                  | `OFF`                         | 禁用 HTTP Range 请求支持（断点续传、分块下载）。如果不需要 HTTP 文件下载功能，关闭此项                                                                                                                     |
| `LWS_WITH_SEQUENCER`               | `OFF`                         | 禁用事件序列器。sequencer 允许按顺序调度一系列回调事件，是一种高级调度机制，大部分应用用不到                                                                                                               |
| `LWS_WITH_SOCKS5`                  | `OFF`                         | 禁用 SOCKS5 代理支持。如果你的网络环境不需要通过 SOCKS5 代理服务器连接，关闭此项                                                                                                                           |
| `LWS_WITH_UNIX_SOCK`               | `OFF`                         | 禁用 Unix Domain Socket 支持。Unix Socket 用于同一台机器上进程间的高效通信，嵌入式 RTOS 场景通常使用 TCP 即可                                                                                              |
| `LWS_WITH_PEER_LIMITS`             | `OFF`                         | 禁用连接数限制功能。服务端用来限制同一 IP 的最大连接数，防止恶意连接。已关闭服务端，此功能无意义                                                                                                           |
| `LWS_WITH_PLUGINS`                 | `OFF`                         | 禁用插件加载机制。允许运行时动态加载 .so 插件扩展功能，嵌入式场景通常编译时就确定所有功能                                                                                                                  |
| `LWS_WITH_HTTP_PROXY`              | `OFF`                         | 禁用 HTTP 正向/反向代理功能。libwebsockets 可以作为 HTTP 代理服务器转发请求，嵌入式场景不需要                                                                                                              |
| `LWS_WITH_ACME`                    | `OFF`                         | 禁用 ACME（Automated Certificate Management Environment）支持。ACME 用于自动申请和续期 TLS 证书（如 Let's Encrypt），嵌入式设备通常预置证书，不需要自动管理                                                |
| `LWS_WITH_JWS`                     | `OFF`                         | 禁用 JWS（JSON Web Signature）支持。JWS 用于签名和验证 JSON 数据，属于 JOSE 规范的一部分，嵌入式场景一般用不到                                                                                             |
| `LWS_WITH_JOSE`                    | `OFF`                         | 禁用 JOSE（JSON Object Signing and Encryption）框架。JOSE 是一组 JSON 安全相关的规范（JWS/JWE/JWK/JWT），关闭后不再编译这些模块                                                                            |
| `LWS_WITH_LEJP`                    | `OFF`                         | 禁用 LEJP（Lightweight Embedded JSON Parser）。这是 libwebsockets 内置的轻量 JSON 解析器，如果你的应用中已经有其他 JSON 库（如 cJSON），可以关闭以减小体积                                                 |
| `LWS_WITH_LEJP_CONF`               | `OFF`                         | 禁用基于 LEJP 的配置文件解析器。用于解析 libwebsockets 的配置文件格式，嵌入式场景通常不使用配置文件                                                                                                        |
| `LWS_WITH_TLS_SESSIONS`            | `OFF`                         | 禁用 TLS 会话缓存和恢复。TLS 会话恢复可以让重复连接时跳过完整握手过程，加快连接速度。关闭后每次连接都进行完整 TLS 握手，牺牲少量连接速度换取更小的体积和内存占用                                           |
| `LWS_WITHOUT_EXTENSIONS`           | `ON`                          | **禁用所有 WebSocket 扩展**。WebSocket 扩展（如 permessage-deflate）在协议握手阶段协商，可对消息进行压缩等处理。`ON` 表示不编译任何扩展，即使 `LWS_WITH_ZLIB=ON` 也不会生效（需注意参数冲突，见下方说明） |
| `LWS_WITH_LIBUV`                   | `OFF`                         | 不使用 libuv 作为事件循环后端。libuv 是跨平台异步 I/O 库（Node.js 的底层），libwebsockets 可以集成它。嵌入式场景使用默认的 poll 事件循环即可                                                               |
| `LWS_WITH_LIBEV`                   | `OFF`                         | 不使用 libev 作为事件循环后端。libev 是高性能事件循环库，嵌入式场景不需要                                                                                                                                  |
| `LWS_WITH_LIBEVENT`                | `OFF`                         | 不使用 libevent 作为事件循环后端。libevent 是网络编程常用的事件库，嵌入式场景不需要                                                                                                                        |
| `LWS_MAX_SMP`                      | `1`                           | 设置最大并发服务线程数为 1。值为 1 时表示单线程模式，去掉所有多线程锁保护代码，减少约 5% 的代码体积和运行时开销。适用于单核 CPU 的嵌入式平台                                                               |

| 参数                          | 节省估算  | 说明                       |
| ------                        | --------- | ------                     |
| `LWS_WITHOUT_SERVER=ON`       | 30%~40%   | 服务端占整个库近一半代码   |
| `LWS_WITHOUT_CLIENT=ON`       | 20%~30%   | 不做客户端时关闭           |
| `LWS_WITHOUT_DEBUG=ON`        | 10%~15%   | 去掉所有调试日志代码       |
| `CMAKE_BUILD_TYPE=MinSizeRel` | 15%~25%   | GCC `-Os` 优化体积而非速度 |
| 关闭 HTTP/CGI/代理等功能      | 10%~15%   | 不需要的协议逐个关         |
| 关闭 JOSE/JWS/ACME            | 5%~10%    | JSON Web/证书自动管理      |

通过 CMake 变量注入更激进的编译优化：

```bash
cmake .. \
    [其他参数...] \
    -DCMAKE_C_FLAGS="-Os -ffunction-sections -fdata-sections" \
    -DCMAKE_EXE_LINKER_FLAGS="-Wl,--gc-sections" \
    -DCMAKE_SHARED_LINKER_FLAGS="-Wl,--gc-sections"
```

| 标志                  | 作用                                                             |
| ------                | ------                                                           |
| `-Os`                 | 优化体积（CMAKE_BUILD_TYPE=MinSizeRel 已包含，但显式指定更保险） |
| `-ffunction-sections` | 每个函数放在独立 section                                         |
| `-fdata-sections`     | 每个变量放在独立 section                                         |
| `-Wl,--gc-sections`   | 链接时丢弃未使用的 section                                       |

**对比参考**

| 配置                  | 大小参考     |
| ------                | ---------    |
| 全功能默认编译        | ~800KB~1MB   |
| 关闭服务端 + 关闭调试 | ~400KB~500KB |
| 最精简客户端配置      | ~200KB~300KB |
| 最精简 + strip        | ~150KB~200KB |
| 最精简 + LTO + strip  | ~100KB~150KB |

### 2.5 验证产出

```bash
ls -lh ${PWD}/../.install/lib/libwebsockets.a
ls -lh ${PWD}/../.install/include/libwebsockets.h

# 检查静态库架构
file ${PWD}/../.install/lib/libwebsockets.a

# 检查 cmake 缓存中的工具链配置（确认是绝对路径）
grep -E "^CMAKE_AR|^CMAKE_RANLIB|^CMAKE_C_COMPILER" CMakeCache.txt
```

## 注意事项

### CMake 版本

libwebsockets 4.5 要求 CMake >= 3.10。Ubuntu 18.04 自带的 CMake 3.10.2 满足要求，但是 Ubuntu 16.04 自带的 CMake 3.8.2 不满足要求，需要升级：

```bash
# 方式一：直接下载二进制包（推荐）
cd /tmp
wget https://github.com/Kitware/CMake/releases/download/v3.22.6/cmake-3.22.6-linux-x86_64.tar.gz
tar -zxvf cmake-3.22.6-linux-x86_64.tar.gz
cp -rf cmake-3.22.6-linux-x86_64/bin/* /usr/local/bin/
cp -rf cmake-3.22.6-linux-x86_64/share/* /usr/local/share/
cmake --version
```

### 不要复用旧的 CMake 缓存

每次修改 cmake 参数后，务必清除缓存再重新配置：

```bash
rm -f CMakeCache.txt
rm -rf CMakeFiles
cmake .. [参数...]
```

---

## 常见问题

| 症状                                                    | 原因                               | 解决方案                                        |
| ------                                                  | ------                             | ----------                                      |
| `unknown type name 'ECDSA_SIG'`                         | OpenSSL 头文件被裁剪，缺少 ecdsa.h | 使用单独编译的完整 OpenSSL 1.1.1t               |
| `Could NOT find OpenSSL`                                | 头文件路径不正确                   | 确认路径下存在 openssl/ssl.h                    |
| `Error running link command: No such file or directory` | CMAKE_AR 使用了工具名而非绝对路径  | 用绝对路径指定 CMAKE_AR 和 CMAKE_RANLIB         |
| `CMake 3.10...4.0 or higher is required`                | 系统 CMake 版本过旧                | 升级 CMake 到 3.10+                             |
| 链接时报 `undefined reference`                          | 静态库链接顺序错误                 | 用 `-Wl,--start-group` / `-Wl,--end-group` 包裹 |
| 连接时找不到库                                          | 动态链接器找不到 .so               | 静态链接方式，不依赖动态库                      |

## 最终产出物

```bash
openssl-OpenSSL_1_1_1-stable/.install/   ← 完整的 OpenSSL（libwebsockets 编译专用）
├── include/openssl/*.h                  ← 完整头文件
└── lib/
    ├── libssl.a
    └── libcrypto.a

libwebsockets-4.5-stable/build/.install/
├── include/
│   ├── libwebsockets.h
│   └── libwebsockets/
└── lib/
    └── libwebsockets.a               ← 最终产出，集成到 RTOS 项目中
```
