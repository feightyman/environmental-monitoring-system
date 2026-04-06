# 🌍 环境监测系统 (Environmental Monitoring System)

本项目是一个完整的物联网（IoT）环境监测与控制系统。项目分为**硬件设备端（下位机）**和**移动应用端（上位机）**两部分。
硬件端基于 STM32 微控制器采集环境数据（如温度、湿度、光照等），并通过 ESP8266 Wi-Fi 模块将数据上传至服务器；移动端基于 UniApp 开发，用户可以通过手机实时查看环境状态，并进行远程设备控制（如开关灯光）。

## 📁 项目结构

项目主要由两个核心文件夹组成：

* **`app/`**: 移动端应用程序源码（基于 UniApp / Vue.js）
* **`environment_project_stm32/`**: 硬件设备端源码（基于 STM32 和 HAL 库）

---

## 🛠️ 硬件设备端 (`environment_project_stm32`)

设备端固件基于 C 语言开发，使用了 STM32CubeMX 工具进行底层配置，并在 Keil MDK-ARM 下进行编译和调试。

### 1. 核心硬件选型
* **主控芯片**: STM32G431RBTx (ARM Cortex-M4)
* **温湿度传感器**: DHT11
* **无线通信模块**: ESP8266 (通过串口进行 AT 指令通信)
* **显示模块**: TFT LCD 屏幕
* **其他外设**: 独立按键、ADC（用于光照等模拟量采集）

### 2. 代码目录说明
核心业务代码与驱动均存放在 `mycode/` 目录下，与自动生成的底层代码分离，便于维护：
* `mycode/DHT11.c & .h`: 温湿度传感器驱动。
* `mycode/esp8266.c & .h`: Wi-Fi 模块网络通信及协议解析。
* `mycode/lcd.c & .h`: LCD 屏幕显示驱动，负责本地 UI 渲染。
* `mycode/key.c & .h`: 实体按键的扫描与处理。
* `Core/`: 包含 `main.c` 以及由 STM32CubeMX 生成的 GPIO, ADC, TIM, USART 等外设初始化代码。
* `MDK-ARM/`: Keil uVision5 工程文件（`environment_project.uvprojx`）。

### 3. 开发与编译环境
* **IDE**: Keil uVision5 (MDK-ARM)
* **配置工具**: STM32CubeMX (`environment_project.ioc`)
* **固件库**: STM32G4xx HAL Driver

---

## 📱 移动应用端 (`app`)

移动端是一个跨平台的应用程序，采用 **UniApp** 框架（Vue.js）开发，可编译为微信小程序、H5、Android 或 iOS App。

### 1. 核心功能
* **实时数据仪表盘**: 图形化展示当前环境的 **温度 (Temperature)**、**湿度 (Humidity)**、**光照强度 (Illumination)** 和 **空气质量 (AQI)**。
* **设备远程控制**: 提供远程开关控制界面（如远程控制灯光 `lamp`）。

### 2. 界面与资源
* `pages/index/index.vue`: 应用的主界面及核心交互逻辑。
* `static/`: 存放应用相关的 UI 图标资源（如 `temp.png`, `humi.png`, `light.png`, `aqi.png` 等）。
* `manifest.json` & `pages.json`: UniApp 项目配置和路由配置文件。

### 3. 开发与编译环境
* **IDE**: HBuilderX / VS Code
* **运行**: 在 HBuilderX 中打开 `app` 目录，点击“运行”即可在内置浏览器或微信开发者工具中预览。

---

## 🚀 快速上手 (Getting Started)

### 硬件端编译与烧录
1. 安装 Keil MDK 5 及对应的 STM32G4 芯片包 (Device Family Pack)。
2. 进入 `environment_project_stm32/MDK-ARM/` 目录，双击打开 `environment_project.uvprojx` 工程。
3. 检查代码中的 Wi-Fi SSID 和密码配置（通常在 `main.c` 或 `esp8266.c` 中）。
4. 连接 ST-Link 或 DAP-Link 下载器，点击编译 (Build) 并烧录 (Download) 到 STM32 核心板中。

### 移动端运行
1. 下载并安装 [HBuilderX](https://www.dcloud.io/hbuilderx.html)。
2. 将 `app` 文件夹拖入 HBuilderX。
3. 确保网络后端服务已开启，或者修改 `app/pages/index/index.vue` 或 `main.js` 中的服务器请求 IP 端口以匹配您的 ESP8266 数据中转节点。
4. 运行到内置浏览器或手机模拟器查看效果。

## 📄 许可证 (License)

本项目硬件驱动部分遵循 STMicroelectronics 提供的 HAL 库协议（位于 `Drivers/STM32G4xx_HAL_Driver/LICENSE.txt`），CMSIS 包含的组件遵循其开源协议。自定义业务代码由作者所有。
