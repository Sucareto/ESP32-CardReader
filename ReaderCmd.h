#ifdef ESP32
#pragma message "当前的开发板是 ESP32"
#define SerialDevice Serial
#define LED_PIN 13
#define PN532_SPI_SS 5

#define SW1_MODE 33
#define SW2_OTA 25
#define SW3_CARD 26
#define SW4_FW 27

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

#define old_fw_version "TN32MSEC003S F/W Ver1.2"
#define old_hw_version "TN32MSEC003S H/W Ver3.0"
#define old_led_info "15084\xFF\x10\x00\x12"

#define new_fw_version "\x94"
#define new_hw_version "837-15396"
#define new_led_info "000-00000\xFF\x11\x40"

bool ReaderMode, FWSW;
uint8_t len, r, checksum;
bool escape = false;
unsigned long ConnectTime = 0;
bool ConnectStatus = false;
uint16_t SleepDelay = 10000;  // ms

#include "FastLED.h"
CRGB leds[8];

#include <SPI.h>
#include <PN532_SPI.h>
PN532_SPI pn532(SPI, PN532_SPI_SS);

#include "PN532.h"
PN532 nfc(pn532);
uint8_t KeyA[6], KeyB[6];
uint8_t DefaultKey[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

#include <U8g2lib.h>
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

#define SPICEAPI_INTERFACE Serial
#include "src/wrappers.h"
spiceapi::Connection CON(512);

static uint8_t mifare_data[][16] = {
  // https://github.com/Sucareto/Arduino-Aime-Reader/blob/main/doc/aime%E7%A4%BA%E4%BE%8B.mct
  { 0x8A, 0x1B, 0x72, 0xE1, 0x02, 0x08, 0x04, 0x00, 0x02, 0xFE, 0x9F, 0xC6, 0xDD, 0x8A, 0x3D, 0x1D },  // 前 4 位是 UID
  {},                                                                                                  // 空数据占位符
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x11, 0x45, 0x14, 0x19, 0x19, 0x81, 0x02, 0x33, 0x33 },  // access code
  { 0x57, 0x43, 0x43, 0x46, 0x76, 0x32, 0x08, 0x77, 0x8F, 0x11, 0x57, 0x43, 0x43, 0x46, 0x76, 0x32 },
};

static unsigned char rf_open[] = {
  0x00, 0x00, 0x00, 0x00, 0x04, 0x20, 0x02, 0x40, 0x02, 0x40, 0x11, 0x88, 0x91, 0x89, 0x49, 0x92,
  0x49, 0x92, 0x91, 0x89, 0x11, 0x88, 0x02, 0x40, 0x02, 0x40, 0x04, 0x20, 0x00, 0x00, 0x00, 0x00
};

static unsigned char rf_off[] = {
  0x01, 0x00, 0x02, 0x00, 0x04, 0x20, 0x0A, 0x40, 0x12, 0x40, 0x21, 0x88, 0x51, 0x89, 0x89, 0x92,
  0x49, 0x93, 0x91, 0x8A, 0x11, 0x8C, 0x02, 0x48, 0x02, 0x50, 0x04, 0x20, 0x00, 0x40, 0x00, 0x80
};

static unsigned char card[] = {
  0x00, 0x00, 0xFC, 0x3F, 0x02, 0x40, 0x32, 0x48, 0x52, 0x48, 0x92, 0x48, 0x12, 0x49, 0x12, 0x4A,
  0x52, 0x48, 0x92, 0x48, 0x12, 0x49, 0x12, 0x4A, 0x12, 0x4C, 0x02, 0x40, 0xFC, 0x3F, 0x00, 0x00
};

static unsigned char blank[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

enum {
  CMD_GET_FW_VERSION = 0x30,
  CMD_GET_HW_VERSION = 0x32,
  // Card read
  CMD_START_POLLING = 0x40,
  CMD_STOP_POLLING = 0x41,
  CMD_CARD_DETECT = 0x42,
  CMD_CARD_SELECT = 0x43,
  CMD_CARD_HALT = 0x44,
  // MIFARE
  CMD_MIFARE_KEY_SET_A = 0x50,
  CMD_MIFARE_AUTHORIZE_A = 0x51,
  CMD_MIFARE_READ = 0x52,
  CMD_MIFARE_WRITE = 0x53,
  CMD_MIFARE_KEY_SET_B = 0x54,
  CMD_MIFARE_AUTHORIZE_B = 0x55,
  // Boot,update
  CMD_TO_UPDATER_MODE = 0x60,
  CMD_SEND_HEX_DATA = 0x61,
  CMD_TO_NORMAL_MODE = 0x62,
  CMD_SEND_BINDATA_INIT = 0x63,
  CMD_SEND_BINDATA_EXEC = 0x64,
  // FeliCa
  CMD_FELICA_PUSH = 0x70,
  CMD_FELICA_THROUGH = 0x71,
  CMD_FELICA_THROUGH_POLL = 0x00,
  CMD_FELICA_THROUGH_READ = 0x06,
  CMD_FELICA_THROUGH_WRITE = 0x08,
  CMD_FELICA_THROUGH_GET_SYSTEM_CODE = 0x0C,
  CMD_FELICA_THROUGH_NDA_A4 = 0xA4,
  // LED board
  CMD_EXT_BOARD_LED = 0x80,
  CMD_EXT_BOARD_LED_RGB = 0x81,
  CMD_EXT_BOARD_LED_RGB_UNKNOWN = 0x82,  // 未知
  CMD_EXT_BOARD_INFO = 0xf0,
  CMD_EXT_FIRM_SUM = 0xf2,
  CMD_EXT_SEND_HEX_DATA = 0xf3,
  CMD_EXT_TO_BOOT_MODE = 0xf4,
  CMD_EXT_TO_NORMAL_MODE = 0xf5,
};

enum {  // 未确认效果
  ERROR_NONE = 0,
  ERROR_NFCRW_INIT_ERROR = 1,
  ERROR_NFCRW_FIRMWARE_UP_TO_DATE = 3,
  ERROR_NFCRW_ACCESS_ERROR = 4,
  ERROR_CARD_DETECT_TIMEOUT = 5,
  ERROR_CARD_DETECT_ERROR = 32,
  ERROR_FELICA_ERROR = 33,
};

typedef union {
  uint8_t bytes[128];
  struct {
    uint8_t frame_len;
    uint8_t addr;
    uint8_t seq_no;
    uint8_t cmd;
    uint8_t payload_len;
    union {
      uint8_t key[6];            // CMD_MIFARE_KEY_SET
      uint8_t color_payload[3];  // CMD_EXT_BOARD_LED_RGB
      struct {                   // CMD_CARD_SELECT,AUTHORIZE,READ
        uint8_t uid[4];
        uint8_t block_no;
      };
      struct {  // CMD_FELICA_THROUGH
        uint8_t encap_IDm[8];
        uint8_t encap_len;
        uint8_t encap_code;
        union {
          struct {  // CMD_FELICA_THROUGH_POLL
            uint8_t poll_systemCode[2];
            uint8_t poll_requestCode;
            uint8_t poll_timeout;
          };
          struct {  // CMD_FELICA_THROUGH_READ,WRITE,NDA_A4
            uint8_t RW_IDm[8];
            uint8_t numService;
            uint8_t serviceCodeList[2];
            uint8_t numBlock;
            uint8_t blockList[1][2];  // CMD_FELICA_THROUGH_READ
            uint8_t blockData[16];    // CMD_FELICA_THROUGH_WRITE
          };
          uint8_t felica_payload[1];
        };
      };
    };
  };
} packet_request_t;

typedef union {
  uint8_t bytes[128];
  struct {
    uint8_t frame_len;
    uint8_t addr;
    uint8_t seq_no;
    uint8_t cmd;
    uint8_t status;
    uint8_t payload_len;
    union {
      uint8_t version[1];  // CMD_GET_FW_VERSION,CMD_GET_HW_VERSION,CMD_EXT_BOARD_INFO
      uint8_t block[16];   // CMD_MIFARE_READ
      struct {             // CMD_CARD_DETECT
        uint8_t count;
        uint8_t type;
        uint8_t id_len;
        union {
          uint8_t mifare_uid[4];
          struct {
            uint8_t IDm[8];
            uint8_t PMm[8];
          };
        };
      };
      struct {  // CMD_FELICA_THROUGH
        uint8_t encap_len;
        uint8_t encap_code;
        uint8_t encap_IDm[8];
        union {
          struct {  // FELICA_CMD_POLL
            uint8_t poll_PMm[8];
            uint8_t poll_systemCode[2];
          };
          struct {
            uint8_t RW_status[2];
            uint8_t numBlock;
            uint8_t blockData[1][1][16];
          };
          uint8_t felica_payload[1];
        };
      };
    };
  };
} packet_response_t;

packet_request_t req;
packet_response_t res;

uint8_t packet_read() {
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
      return req.cmd;
    }
    checksum += r;
  }
  return 0;
}

void packet_write() {
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

void res_init(uint8_t payload_len = 0) {
  res.frame_len = 6 + payload_len;
  res.addr = req.addr;
  res.seq_no = req.seq_no;
  res.cmd = req.cmd;
  res.status = ERROR_NONE;
  res.payload_len = payload_len;
}

void sys_to_normal_mode() {
  res_init();
  if (nfc.getFirmwareVersion()) {
    res.status = ERROR_NFCRW_FIRMWARE_UP_TO_DATE;
    u8g2.drawXBM(95, 0, 16, 16, blank);
    u8g2.drawXBM(113, 0, 16, 16, blank);
  } else {
    res.status = ERROR_NFCRW_INIT_ERROR;
    u8g2.drawXBM(95, 0, 16, 16, rf_off);
    FastLED.showColor(0xFF0000);
  }
}

void sys_get_fw_version() {
  if (FWSW) {
    res_init(sizeof(old_fw_version) - 1);
    memcpy(res.version, old_fw_version, res.payload_len);
  } else {
    res_init(sizeof(new_fw_version) - 1);
    memcpy(res.version, new_fw_version, res.payload_len);
  }
}

void sys_get_hw_version() {
  if (FWSW) {
    res_init(sizeof(old_hw_version) - 1);
    memcpy(res.version, old_hw_version, res.payload_len);
  } else {
    res_init(sizeof(new_hw_version));
    memcpy(res.version, new_hw_version, res.payload_len);
  }
}

void sys_get_led_info() {
  if (FWSW) {
    res_init(sizeof(old_led_info) - 1);
    memcpy(res.version, old_led_info, res.payload_len);
  } else {
    res_init(sizeof(new_led_info) - 1);
    memcpy(res.version, new_led_info, res.payload_len);
  }
}


void nfc_start_polling() {
  res_init();
  nfc.setRFField(0x00, 0x01);
  u8g2.drawXBM(95, 0, 16, 16, rf_open);
}

void nfc_stop_polling() {
  res_init();
  nfc.setRFField(0x00, 0x00);
  u8g2.drawXBM(95, 0, 16, 16, blank);
}

void nfc_card_detect() {
  uint16_t SystemCode;
  uint8_t bufferLength;
  if (!digitalRead(SW3_CARD)) {
    memcpy(res.mifare_uid, mifare_data[0], 0x04);
    res.id_len = 0x04;
    res_init(0x07);
    res.count = 1;
    res.type = 0x10;
  } else if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, res.mifare_uid, &res.id_len) && nfc.getBuffer(&bufferLength)[4] == 0x08) {  // Only read cards with sak=0x08
    res_init(0x07);
    res.count = 1;
    res.type = 0x10;
  } else if (nfc.felica_Polling(0xFFFF, 0x00, res.IDm, res.PMm, &SystemCode, 200) == 1) {
    res_init(0x13);
    res.count = 1;
    res.type = 0x20;
    res.id_len = 0x10;
  } else {
    res_init(1);
    res.count = 0;
    res.status = ERROR_NONE;
    u8g2.drawXBM(113, 0, 16, 16, blank);
    return;
  }
  u8g2.drawXBM(113, 0, 16, 16, card);
}

void nfc_mifare_authorize_a() {
  res_init();
  if (!nfc.mifareclassic_AuthenticateBlock(req.uid, 4, req.block_no, 0, KeyA)) {
    res.status = ERROR_NFCRW_ACCESS_ERROR;
  }
}

void nfc_mifare_authorize_b() {
  res_init();
  if (!nfc.mifareclassic_AuthenticateBlock(req.uid, 4, req.block_no, 1, KeyB)) {
    res.status = ERROR_NFCRW_ACCESS_ERROR;
  }
}

void nfc_mifare_read() {
  res_init(0x10);
  if (!digitalRead(SW3_CARD)) {
    memcpy(res.block, mifare_data[req.block_no], 16);
    res_init(0x10);
    return;
  } else if (!nfc.mifareclassic_ReadDataBlock(req.block_no, res.block)) {
    res_init();
    res.status = ERROR_CARD_DETECT_TIMEOUT;  // TODO
  }
}

void nfc_felica_through() {
  uint16_t SystemCode;
  if (nfc.felica_Polling(0xFFFF, 0x01, res.encap_IDm, res.poll_PMm, &SystemCode, 200) == 1) {
    SystemCode = SystemCode >> 8 | SystemCode << 8;
  } else {
    res_init();
    res.status = ERROR_FELICA_ERROR;
    return;
  }
  uint8_t code = req.encap_code;
  res.encap_code = code + 1;
  switch (code) {
    case CMD_FELICA_THROUGH_POLL:
      {
        res_init(0x14);
        res.poll_systemCode[0] = SystemCode;
        res.poll_systemCode[1] = SystemCode >> 8;
      }
      break;
    case CMD_FELICA_THROUGH_GET_SYSTEM_CODE:
      {
        res_init(0x0D);
        res.felica_payload[0] = 0x01;
        res.felica_payload[1] = SystemCode;
        res.felica_payload[2] = SystemCode >> 8;
      }
      break;
    case CMD_FELICA_THROUGH_NDA_A4:
      {
        res_init(0x0B);
        res.felica_payload[0] = 0x00;
      }
      break;
    case CMD_FELICA_THROUGH_READ:
      {
        uint16_t serviceCodeList[1] = { (uint16_t)(req.serviceCodeList[1] << 8 | req.serviceCodeList[0]) };
        for (uint8_t i = 0; i < req.numBlock; i++) {
          uint16_t blockList[1] = { (uint16_t)(req.blockList[i][0] << 8 | req.blockList[i][1]) };
          if (nfc.felica_ReadWithoutEncryption(1, serviceCodeList, 1, blockList, res.blockData[i]) != 1) {
            memset(res.blockData[i], 0, 16);  // dummy data
          }
        }
        res.RW_status[0] = 0;
        res.RW_status[1] = 0;
        res.numBlock = req.numBlock;
        res_init(0x0D + req.numBlock * 16);
      }
      break;
    case CMD_FELICA_THROUGH_WRITE:
      {
        res_init(0x0C);  // WriteWithoutEncryption,ignore
        res.RW_status[0] = 0;
        res.RW_status[1] = 0;
      }
      break;
    default:
      res_init();
      res.status = ERROR_FELICA_ERROR;
  }
  res.encap_len = res.payload_len;
}
