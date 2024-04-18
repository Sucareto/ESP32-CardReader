#include "Aime_Reader.h"

void (*ReaderMain)();

void setup() {
  pinMode(SW1_MODE, INPUT_PULLUP);  // Switch mode
  pinMode(SW3_CARD, INPUT_PULLUP);  // Hardcode mifare
  pinMode(SW4_FW, INPUT_PULLUP);    // (Aime) Baudrate & fw/hw | (Spice) 1P 2P
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_mf);
  u8g2.clearBuffer();
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, 8);
  FastLED.setBrightness(20);  // LED brightness
  FastLED.showColor(0);

#ifdef OTA_Enable                  // update check
  pinMode(SW2_OTA, INPUT_PULLUP);  // Enable OTA
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

  // mode select
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
    u8g2.drawStr(0, 64, FWSW ? "TN32MSEC003S" : new_hw_version);
    FastLED.showColor(FWSW ? CRGB::Green : CRGB::Blue);
    ReaderMain = AimeCardReader;
  }

  memset(req.bytes, 0, sizeof(req.bytes));
  memset(res.bytes, 0, sizeof(res.bytes));
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
      && nfc.mifareclassic_AuthenticateBlock(res.mifare_uid, res.id_len, 1, 0, DefaultKey)
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
    case 0:
      return;
    case CMD_TO_NORMAL_MODE:
      sys_to_normal_mode();
      break;
    case CMD_GET_FW_VERSION:
      sys_get_fw_version();
      break;
    case CMD_GET_HW_VERSION:
      sys_get_hw_version();
      break;
    // Card read
    case CMD_START_POLLING:
      nfc_start_polling();
      break;
    case CMD_STOP_POLLING:
      nfc_stop_polling();
      break;
    case CMD_CARD_DETECT:
      nfc_card_detect();
      break;
    // MIFARE
    case CMD_MIFARE_KEY_SET_A:
      memcpy(KeyA, req.key, 6);
      res_init();
      break;
    case CMD_MIFARE_KEY_SET_B:
      res_init();
      memcpy(KeyB, req.key, 6);
      break;
    case CMD_MIFARE_AUTHORIZE_A:
      nfc_mifare_authorize_a();
      break;
    case CMD_MIFARE_AUTHORIZE_B:
      nfc_mifare_authorize_b();
      break;
    case CMD_MIFARE_READ:
      nfc_mifare_read();
      break;
    // FeliCa
    // case CMD_FELICA_THROUGH:
    //   nfc_felica_through();
    //   break;
    // LED
    case CMD_EXT_BOARD_LED_RGB:
      FastLED.showColor(CRGB(req.color_payload[0], req.color_payload[1], req.color_payload[2]));
      break;
    case CMD_EXT_BOARD_INFO:
      sys_get_led_info();
      break;
    case CMD_EXT_BOARD_LED_RGB_UNKNOWN:
      break;
    case CMD_CARD_SELECT:
    case CMD_CARD_HALT:
    case CMD_EXT_TO_NORMAL_MODE:
    case CMD_TO_UPDATER_MODE:
    case CMD_SEND_HEX_DATA:
      res_init();
      break;

    case STATUS_SUM_ERROR:
      res_init();
      res.status = STATUS_SUM_ERROR;
      break;

    default:
      res_init();
      res.status = STATUS_INVALID_COMMAND;
  }
  u8g2.sendBuffer();
  ConnectTime = millis();
  packet_write();
}
