# ESP32-CardReader

- 此项目为 [Arduino-Aime-Reader](https://github.com/Sucareto/Arduino-Aime-Reader) 的使用示例。
- 主控模块是 NodeMCU-32S，添加了 SSD1306 模块显示信息。
- 可以使用拨码开关切换连接波特率和修改其他设置，也可以切换为 HSU 直通模式。
- 直通模式可以给 Android 和 PC 连接作为 USB PN532 读卡器使用，例如使用 [MIFARE Classic Tool (MCT)](https://github.com/ikarus23/MifareClassicTool) 或 [MifareOneTool](https://github.com/xcicode/MifareOneTool) 之类的软件。

## 实物图：
<details><summary>点击展开</summary>
  
![读卡器](https://user-images.githubusercontent.com/28331534/170975617-4c0de22a-8daa-4263-a6b7-a09e974af1d3.jpg)
![拨码开关](https://user-images.githubusercontent.com/28331534/170975647-94706142-f535-4d15-8fc0-86bf5a60257b.jpg)

https://user-images.githubusercontent.com/28331534/170975661-137f3474-f61a-4a4d-8ec2-b13b3c165761.mp4

</details>

## 使用说明：

### PCB 设计：
因为不舍得花时间去画 PCB，所以 ESP32 和 PN532 就使用了模块直插的方式，或许以后有机会的话...

### 拨码开关：
在 PCB 放置了两个 4P 拨码开关。

#### SW 1-4：（可修改代码自定义）
- SW1：切换波特率，对应`high_baudrate` 
- SW2：切换固件显示版本，`TN32MSEC003S` 或 `837-15396`
- SW3：切换 BLE 连接（未实现）
- SW4：启用 OTA 更新

#### TTL：（用于切换至 “USB PN532 读卡器”模式）
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
- 操作 SSD1306：[u8g2](https://github.com/olikraus/u8g2)
- OLED 显示图案设计：PCtoLCD2002
- AimePad 盖板设计：[CoolBreezeArcanine](https://github.com/CoolBreezeArcanine)
- PCB 图案：[妖夢 - ぷりん](https://www.pixiv.net/artworks/87578487)
- OTA 代码参考：[OTA Updates](https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html#http-server)
