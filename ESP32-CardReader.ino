#ifdef ARDUINO_NodeMCU_32S
#pragma message "当前的开发板是 NodeMCU_32S"
#define SerialDevice Serial
#define LED_PIN 13
#define PN532_SPI_SS 5


#define SW1_MODE 33
#define SW2_OTA 25
#define SW3_LED 26
#define SW4_FW 27
bool ReaderMode, FWSW;

// #define OTA_Enable
#ifdef OTA_Enable
#pragma message "已开启 OTA 更新功能"
#define STASSID "SSIDNAME"
#define STAPASS "PASSWORD"
#define OTA_URL "http://esp-update.local/Sucareto/ESP32-Reader:2333/"
#include <WiFi.h>
#include <HTTPUpdate.h>
#endif

#else
#error "未适配的开发板！！！"
#endif

#include "ReaderCmd.h"
void (*ReaderMain)();
unsigned long ConnectTime = 0;
bool ConnectStatus = false;
uint16_t SleepDelay = 10000;  // ms

void setup() {
  pinMode(SW1_MODE, INPUT_PULLUP);  // Switch mode
  pinMode(SW2_OTA, INPUT_PULLUP);   // Enable OTA
  pinMode(SW3_LED, INPUT_PULLUP);   // LED brightness
  pinMode(SW4_FW, INPUT_PULLUP);    // (Aime) Baudrate & fw/hw | (Spice) 1P 2P

  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_mf);
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, 8);
  if (digitalRead(SW3_LED)) {
    FastLED.setBrightness(20);
  }
  FastLED.showColor(0);

#ifdef OTA_Enable
  if (!digitalRead(SW2_OTA)) {
    WiFi.begin(STASSID, STAPASS);
    u8g2.drawStr(0, 28, "WiFi Connecting...");
    u8g2.sendBuffer();
    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
    };
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
  }
#endif

  nfc.begin();
  while (!nfc.getFirmwareVersion()) {
    u8g2.drawXBM(95, 0, 16, 16, rf_off);
    u8g2.sendBuffer();
    FastLED.showColor(0xFF0000);
    delay(500);
    FastLED.showColor(0);
    delay(500);
  }
  nfc.setPassiveActivationRetries(0x10);
  nfc.SAMConfig();
  u8g2.clearBuffer();

  ReaderMode = !digitalRead(SW1_MODE);
  FWSW = !digitalRead(SW4_FW);
  if (ReaderMode) {  // BEMANI mode
    SerialDevice.begin(115200);
    u8g2.drawStr(0, 14, "SpiceTools");
    u8g2.drawStr(117, 64, FWSW ? "2P" : "1P");
    FastLED.showColor(CRGB::Yellow);
    ReaderMain = SpiceToolsReader;
  } else {  // Aime mode
    SerialDevice.begin(FWSW ? 38400 : 115200);
    u8g2.drawStr(0, 16, "Aime Reader");
    u8g2.drawStr(105, 64, FWSW ? "LOW" : "HIGH");
    u8g2.drawStr(0, 64, FWSW ? "TN32MSEC003S" : "837-15396");
    FastLED.showColor(FWSW ? CRGB::Green : CRGB::Blue);
    ReaderMain = AimeCardReader;
  }
  memset(&req, 0, sizeof(req.bytes));
  memset(&res, 0, sizeof(res.bytes));
  u8g2.sendBuffer();
  ConnectTime = millis();
  ConnectStatus = true;
}


void loop() {
  ReaderMain();
  if (ConnectStatus) {
    if ((millis() - ConnectTime) > SleepDelay) {
      u8g2.sleepOn();
      ConnectStatus = false;
    }
  } else {
    if ((millis() - ConnectTime) < SleepDelay) {
      u8g2.sleepOff();
      ConnectStatus = true;
    }
  }
}


void SpiceToolsReader() {  // Spice mode
  uint16_t SystemCode;
  char card_id[17];
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, res.mifare_uid, &res.id_len)
      && nfc.mifareclassic_AuthenticateBlock(res.mifare_uid, res.id_len, 1, 0, MifareKey)
      && nfc.mifareclassic_ReadDataBlock(1, res.block)) {
    sprintf(card_id, "%02X%02X%02X%02X%02X%02X%02X%02X",
            res.block[0], res.block[1], res.block[2], res.block[3],
            res.block[4], res.block[5], res.block[6], res.block[7]);

  } else if (nfc.felica_Polling(0xFFFF, 0x00, res.IDm, res.PMm, &SystemCode, 200) == 1) {
    sprintf(card_id, "%02X%02X%02X%02X%02X%02X%02X%02X",
            res.IDm[0], res.IDm[1], res.IDm[2], res.IDm[3],
            res.IDm[4], res.IDm[5], res.IDm[6], res.IDm[7]);
  } else {
    return;
  }
  u8g2.drawXBM(113, 0, 16, 16, card);
  u8g2.sendBuffer();
  spiceapi::InfoAvs avs_info{};
  if (spiceapi::info_avs(CON, avs_info)) {
    FWSW = !digitalRead(SW4_FW);
    spiceapi::card_insert(CON, FWSW, card_id);
    u8g2.drawStr(0, 30, card_id);
    u8g2.drawStr(0, 64, (avs_info.model + ":" + avs_info.dest + avs_info.spec + avs_info.rev + ":" + avs_info.ext).c_str());
    u8g2.drawStr(117, 64, FWSW ? "2P" : "1P");
    u8g2.sendBuffer();
    for (int i = 0; i < 8; i++) {
      leds[i] = CRGB::Red;
      leds[7 - i] = CRGB::Blue;
      FastLED.delay(50);
      leds[i] = CRGB::Black;
      leds[7 - i] = CRGB::Black;
    }
    FastLED.show();
  }
  u8g2.drawXBM(113, 0, 16, 16, blank);
  u8g2.drawStr(0, 30, "                ");
  u8g2.sendBuffer();
  ConnectTime = millis();
}


void AimeCardReader() {  // Aime mode
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
      sg_res_init();
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
      return;
    default:
      sg_res_init();
  }
  ConnectTime = millis();
  packet_write();
}
