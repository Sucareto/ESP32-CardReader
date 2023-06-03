# ESP32-CardReader

- 此项目为 [Arduino-Aime-Reader](https://github.com/Sucareto/Arduino-Aime-Reader) 的使用示例，使用的主控模块是 NodeMCU-32S
- 添加了 SSD1306 模块展示状态
- 添加了拨码开关用于切换读卡器模式和波特率，也可以切换为 HSU 直通模式
  - 直通模式可以给 Android 或 PC 连接作为 USB PN532 读卡器使用，例如使用 [MIFARE Classic Tool (MCT)](https://github.com/ikarus23/MifareClassicTool) 或 [MifareOneTool](https://github.com/xcicode/MifareOneTool) 等读卡软件
- 在原版 Aime 读卡器的基础上添加了 SpiceTools 模式，可以读取自定义的 MIFARE 卡和 FeliCa 卡
  - MIFARE 读取 sector 0 block 1 的前半部分作为卡号发送（可以通过代码定义）
  - FeliCa 读取 idm 作为卡号发送
  - **仅用于分辨卡片和账号，读卡逻辑和卡号计算未正确实现**

**本项目仅用于展示原项目 [Arduino-Aime-Reader](https://github.com/Sucareto/Arduino-Aime-Reader) 的使用示例，未曾开展或参与成品售卖，也不会向商业化制作提供任何支持。**


## 实物图：
<details><summary>点击展开</summary>
  
![读卡器](https://user-images.githubusercontent.com/28331534/170975617-4c0de22a-8daa-4263-a6b7-a09e974af1d3.jpg)
![拨码开关](https://user-images.githubusercontent.com/28331534/170975647-94706142-f535-4d15-8fc0-86bf5a60257b.jpg)

https://user-images.githubusercontent.com/28331534/170975661-137f3474-f61a-4a4d-8ec2-b13b3c165761.mp4

</details>

## 使用说明：

### PCB 设计：
- 该 PCB 方案仅用于我自己快速测试，并非最佳实现方案
- 如果需要使用此 PCB 方案，请务必把 logo 删掉，然后根据需求认真审查 PCB 设计并作出修改后再进行制作


### 拨码开关：
在 PCB 放置了两个 4P 拨码开关：

#### SW 1-4：（用于更改读卡器功能，切换后需要重启）
- SW1：切换读卡器模式
  - ON：SpiceTools 模式，需要在 SpiceTools 添加启动参数 `-apiserial COM1 -apiserialbaud 115200`，"COM1" 改为实际的端口号
  - OFF：Aime 模式，和 [Arduino-Aime-Reader](https://github.com/Sucareto/Arduino-Aime-Reader) 使用方法一致
- SW2：切换启动时的 OTA 开关
  - ON：连接 WiFi 获取更新，如未能连接到 WiFi 则会持续到连接成功后才能启动
  - OFF：跳过检查更新，直接启动
- SW3：无卡测试模式，适用于在没有卡片的情况下模拟读取代码指定的卡，例如授权卡
  - ON：跳过读卡，使用硬编码的 `mifare_data` 数据
  - OFF：读取实际卡片
- SW4：不同模式的功能切换
  - Spice 模式（可在运行中切换）
    - ON：卡号发送到 2P 槽位（需要游戏支持）
    - OFF：卡号发送到 1P 槽位
  - Aime 模式
    - ON：使用 38400 波特率初始化，固件版本是 TN32MSEC003S
    - OFF：使用 115200 波特率初始化，固件版本是 837-15396

#### TTL：（用于切换至 “USB PN532 读卡器”模式，需要在断电情况下切换）
- SW1：连接到 ESP32 的 `EN` 引脚，调为 `ON` 后会停用 ESP32，只使用 CH340 串口通信芯片
- SW2：在目前版本（v2）为空置
- SW3 & SW4：连接 CH340 和 PN532 的 `RX TX`引脚

此处是直接与 CH340通信，所以不需要反转；如果正常使用 ESP32 不断开这两个引脚，会影响串口信息接收。  
把 TTL 的拨码开关全部调为 `ON` 后，PN532 的拨码开关也要设置为 `HSU` 模式，重新通电即可使用。

### OTA 更新：
在代码里修改以下定义：
- `STASSID`：WIFI 名
- `STAPSK`：WIFI 密码
- `OTA_URL`：更新包下载地址，可以使用文件地址，或者使用 [index.php](OTA/index.php) 来控制是否需要更新

[index.php](OTA/index.php) 默认是判断文件 MD5 是否一致，不一致就会发送更新。

## 感谢：

- 原项目：[Arduino-Aime-Reader](https://github.com/Sucareto/Arduino-Aime-Reader)
- 驱动 WS2812B：[FastLED](https://github.com/FastLED/FastLED)
- 操作 SSD1306：[u8g2](https://github.com/olikraus/u8g2)
- Spice 通信（已复制到本仓库并根据需要修改了部分代码）：[SpiceAPI](https://github.com/spicetools/spicetools/tree/master/api/resources/arduino) 
- 驱动 PN532（已复制到本仓库并根据需要修改了部分代码）：[PN532](https://github.com/elechouse/PN532)
- OLED 显示图案设计：PCtoLCD2002
- AimePad 盖板设计：[CoolBreezeArcanine](https://github.com/CoolBreezeArcanine)
- PCB 图案：[妖夢 - ぷりん](https://www.pixiv.net/artworks/87578487)
- OTA 代码参考：[OTA Updates](https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html#http-server)
- Spice 通信参考：[PN5180-cardio SpiceAPI branch](https://github.com/CrazyRedMachine/PN5180-cardio/tree/SpiceAPI)
