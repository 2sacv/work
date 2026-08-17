### 星闪SDK移植使用说明:

----

#### 一、星闪SDK源码

1. 解压 ws73_sdk_linux_WS73_1.10.110.tar.gz，目录如下

   ```shell
   application  
   build  
   driver  
   firmware  
   include  
   Makefile
   output
   open_source  
   
   # application、open_source 主要是Demo及 wpa_supplicant的源码，不拷贝到dirver目录
   # ./application/lib/ 目录下有各种处理器相关libble_host.a，蓝牙配网需要用到海康蓝牙私有协议，需要拷贝libble_host.a编译jco_server
   ```

   

#### 二、编译裁剪说明

 1. 编译配置文件

    `ws73_usb_light_v2_jco.config` copy from ws73_usb_light_v2.config

 2. 修改编译器

    ```
    WSCFG_CROSS_COMPILE="arm-v01c02-linux-musleabi-"
    WSCFG_KERNEL_DIR=$(KERNEL_DIR)
    WSCFG_EXTRA_CFLAGS=""
    WSCFG_EXTRA_PARAMS=""
    # WSCFG_ARCH_ARM is not set
    WSCFG_ARCH_CUSTOM=y
    WSCFG_ARCH_NAME="arm"
    ```

 3. GPIO适配

    管脚不在驱动做配置，全部屏蔽

    ```
    # CONFIG_INI_HOST_GPIO=17  #PA17 for t32
    # CONFIG_INI_HOST_GPIO_POWER_ON_LEVEL=1
    # CONFIG_INI_DEVICE_AWAKE_GPIO_IDX=10
    # CONFIG_INI_DEVICE_AWAKE_GPIO_LEVEL=1
    # CONFIG_INI_WAKE_UP_GPIO_IDX=70
    # CONFIG_INI_WAKE_UP_GPIO_LEVEL=1
    # CONFIG_INI_PLAT_REBOOT_TYPE_GPIO=0
    ```

 4. 其它配置

    ```shell
    # WIFI_ALG_CCA is not set           # CCA调测
    # WIFI_ALG_TEMP_PROTECT is not set  # 温度保护，只保留最基础的温度保护
    # WIFI_ALG_TXBF is not set
    # WIFI_ALG_EDCA is not set
    
    ###
    ### TXBF (Transmit Beamforming)
    ### Transmit Beamforming 是一种用于优化 Wi-Fi 信号传输的技术。它主要是通过智能地调整发射端的天线阵列的相位和幅度，从而使信号在接收端聚焦，提高信号### 质量和接收灵敏度
    
    ### EDCA 是 Wi-Fi 网络中的一种 访问控制机制，主要用于管理无线信道的访问。它是在 IEEE 802.11e 标准中引入的，用来改进 DCA (Distributed Channel ### Access) 机制，增加了更多的服务质量（QoS）控制，以保证不同类型流量的优先级。
    
    ### wifi 蓝牙驱动共存
    WIFI_BTCOEX=y
    
    ### 配置 CONFIG_PLAT_DFR_OUTPUT_PATH 需要保证该路径随时可写，以便出现系统 panic
    ### 时，能正常保存日志，暂不打开
    # CONFIG_PLAT_SUPPORT_DFR=y
    # CONFIG_PLAT_DFR_OUTPUT_PATH="/tmp"
    
    ### 配置文件路径
    CONFIG_FIRMWARE_BIN_PATH="/ipc/drv/airlink/ws73/ws73.bin"
    CONFIG_FIRMWARE_WIFICALI_PATH="/ipc/drv/airlink/ws73/wifi_cali.bin"
    CONFIG_FIRMWARE_BSLECALI_PATH="/ipc/drv/airlink/ws73/btc_cali.bin"
    CONFIG_FIRMWARE_WOW_PATH="/ipc/drv/airlink/ws73/wow.bin"
    CONFIG_INI_FILE_PATH="/ipc/drv/airlink/ws73_cfg.ini"
    
    
    ### 蓝牙相关功能，参考 ws73_usb_light.config
    #
    # BLE
    #
    WSCFG_BLE_COMPILE_BY_DEFAULT=y
    
    #
    # Configure the ble ini file
    #
    # CONFIG_INI_BLE_DISABLE_LL_PRIVACY is not set
    CONFIG_INI_BLE_TV_RC_SCAN_INTERVAL=480
    CONFIG_INI_BLE_TV_RC_SCAN_WINDOW=48
    # CONFIG_INI_BSLE_FRONT_SWITCH is not set
    # end of Configure the ble ini file
    
    CONFIG_BLE_MAC_FORK=y
    # end of BLE
    ```

5. 拷贝驱动及配置文件

   ```
   airlink/
   ├── [ 17K]  ble_soc.ko
   ├── [214K]  plat_soc.ko
   ├── [791K]  wifi_soc.ko
   ├── [4.0K]  ws73
   │   ├── [7.6K]  btc_cali.bin
   │   ├── [ 21K]  wifi_cali.bin
   │   └── [141K]  ws73.bin
   └── [ 16K]  ws73_cfg.ini
   ```

   



