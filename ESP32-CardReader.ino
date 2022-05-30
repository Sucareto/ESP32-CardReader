#if defined(ARDUINO_NodeMCU_32S)
#pragma message "当前的开发板是 NodeMCU_32S"
#define SerialDevice Serial
#define LED_PIN 13
#define PN532_SPI_SS 5

#define SW1 33
#define SW2 25
#define SW3 26
#define SW4 27

#pragma message "已启用OTA更新"

#define STASSID "ALL.Net Wi-Fi"
#define STAPASS "SEGASEGASEGA"
#define OTA_URL "http://esp-update.local/Sucareto/ESP32-Reader:2333/"
#include <WiFi.h>
#include <HTTPUpdate.h>

#else
#error "开发板选错了！！！"
#endif

#include "Aime-Reader.h"

void setup() {
  pinMode(SW1, INPUT_PULLUP);
  pinMode(SW2, INPUT_PULLUP);
  pinMode(SW3, INPUT_PULLUP);
  pinMode(SW4, INPUT_PULLUP);

  bool high_baudrate = !digitalRead(SW1);
  FWSW = !digitalRead(SW2);
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_mf);
  u8g2.drawXBM(0, 0, 16, 16, usb_disconnect);
  u8g2.drawStr(48, 10, high_baudrate ? "HIGH" : "LOW");
  u8g2.sendBuffer();

  SerialDevice.begin(high_baudrate ? 115200 : 38400);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, 8);
  if (!digitalRead(SW3)) {
    FastLED.setBrightness(20);
  }
  FastLED.showColor(0);

  if (!digitalRead(SW4)) {
    WiFi.begin(STASSID, STAPASS);
    u8g2.drawStr(0, 28, "WiFi Connecting...");
    u8g2.sendBuffer();
    while (WiFi.status() != WL_CONNECTED);
    u8g2.drawStr(0, 28, "WiFi connected.          ");
    u8g2.drawStr(0, 41, "Check update...          ");
    u8g2.sendBuffer();
    WiFiClient client;
    httpUpdate.setLedPin(LED_BUILTIN, LOW);
    t_httpUpdate_return ret = httpUpdate.update(client, OTA_URL);
    switch (ret) {
      case HTTP_UPDATE_FAILED:
        u8g2.drawStr(0, 41, "Check failed.          ");
        break;

      case HTTP_UPDATE_NO_UPDATES:
        u8g2.drawStr(0, 41, "Already up to date.          ");
        break;
    }
    u8g2.sendBuffer();
  }

  nfc.begin();
  while (!nfc.getFirmwareVersion()) {
    u8g2.drawXBM(20, 0, 16, 16, rf_off);
    u8g2.sendBuffer();
    FastLED.showColor(0xFF0000);
    delay(500);
    FastLED.showColor(0);
    delay(500);
  }
  nfc.setPassiveActivationRetries(0x10);//设定等待次数
  nfc.SAMConfig();

  memset(&req, 0, sizeof(req.bytes));
  memset(&res, 0, sizeof(res.bytes));

  u8g2.drawXBM(20, 0, 16, 16, blank);
  u8g2.drawStr(0, 64, FWSW ? "TN32MSEC003S" : "837-15396");
  u8g2.sendBuffer();
  FastLED.showColor(high_baudrate ? 0x0000FF : 0x00FF00);

}

unsigned long ConnectTime = 0;
bool ConnectStatus = false;

void loop() {
  SerialCheck();
  packet_write();
  if (ConnectStatus & millis() - ConnectTime > 5000) {
    u8g2.drawXBM(0, 0, 16, 16, usb_disconnect);
    u8g2.sendBuffer();
    ConnectStatus = false;
  }
}

static uint8_t len, r, checksum;
static bool escape = false;

static uint8_t packet_read() {
  while (SerialDevice.available()) {
    r = SerialDevice.read();
    if (r == 0xE0) {
      req.frame_len = 0xFF;
      continue;
    }
    if (req.frame_len == 0xFF) {
      req.frame_len = r;
      len = 0;
      checksum = r;
      continue;
    }
    if (r == 0xD0) {
      escape = true;
      continue;
    }
    if (escape) {
      r++;
      escape = false;
    }
    req.bytes[++len] = r;
    if (len == req.frame_len && checksum == r) {
      ConnectStatus = true;
      ConnectTime = millis();
      return req.cmd;
    }
    checksum += r;
  }
  return 0;
}

static void packet_write() {
  uint8_t checksum = 0, len = 0;
  if (res.cmd == 0) {
    return;
  }
  SerialDevice.write(0xE0);
  while (len <= res.frame_len) {
    uint8_t w;
    if (len == res.frame_len) {
      w = checksum;
    } else {
      w = res.bytes[len];
      checksum += w;
    }
    if (w == 0xE0 || w == 0xD0) {
      SerialDevice.write(0xD0);
      SerialDevice.write(--w);
    } else {
      SerialDevice.write(w);
    }
    len++;
  }
  res.cmd = 0;
}

void SerialCheck() {
  switch (packet_read()) {
    case SG_NFC_CMD_RESET:
      sg_nfc_cmd_reset();
      break;
    case SG_NFC_CMD_GET_FW_VERSION:
      sg_nfc_cmd_get_fw_version();
      break;
    case SG_NFC_CMD_GET_HW_VERSION:
      sg_nfc_cmd_get_hw_version();
      break;
    case SG_NFC_CMD_POLL:
      sg_nfc_cmd_poll();
      break;
    case SG_NFC_CMD_MIFARE_READ_BLOCK:
      sg_nfc_cmd_mifare_read_block();
      break;
    case SG_NFC_CMD_FELICA_ENCAP:
      sg_nfc_cmd_felica_encap();
      break;
    case SG_NFC_CMD_AIME_AUTHENTICATE:
      sg_nfc_cmd_aime_authenticate();
      break;
    case SG_NFC_CMD_BANA_AUTHENTICATE:
      sg_nfc_cmd_bana_authenticate();
      break;
    case SG_NFC_CMD_MIFARE_SELECT_TAG:
      sg_nfc_cmd_mifare_select_tag();
      break;
    case SG_NFC_CMD_MIFARE_SET_KEY_AIME:
      sg_nfc_cmd_mifare_set_key_aime();
      break;
    case SG_NFC_CMD_MIFARE_SET_KEY_BANA:
      sg_nfc_cmd_mifare_set_key_bana();
      break;
    case SG_NFC_CMD_RADIO_ON:
      sg_nfc_cmd_radio_on();
      break;
    case SG_NFC_CMD_RADIO_OFF:
      sg_nfc_cmd_radio_off();
      break;
    case SG_RGB_CMD_RESET:
      sg_led_cmd_reset();
      break;
    case SG_RGB_CMD_GET_INFO:
      sg_led_cmd_get_info();
      break;
    case SG_RGB_CMD_SET_COLOR:
      sg_led_cmd_set_color();
      break;
    case 0:
      break;
    default:
      sg_res_init();
  }
}
