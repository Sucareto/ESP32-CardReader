#include <SPI.h>
#include <PN532_SPI.h>
PN532_SPI pn532(SPI, PN532_SPI_SS);

#include "PN532.h"
PN532 nfc(pn532);

#include "FastLED.h"
CRGB leds[8];

#include <U8g2lib.h>
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

static unsigned char usb_disconnect[] = {
  0x00, 0x40, 0x00, 0xAE, 0x00, 0x51, 0x80, 0x20, 0x00, 0x41, 0x80, 0x42, 0x40, 0x44, 0x08, 0x28,
  0x14, 0x14, 0x22, 0x02, 0x42, 0x00, 0x82, 0x00, 0x04, 0x01, 0x8A, 0x00, 0x75, 0x00, 0x02, 0x00
};

static unsigned char usb_connect[] = {
  0x00, 0x40, 0x00, 0xA0, 0x00, 0x51, 0x80, 0x2A, 0x60, 0x14, 0x50, 0x08, 0xB0, 0x10, 0x48, 0x21,
  0x84, 0x12, 0x08, 0x0D, 0x10, 0x0A, 0x28, 0x06, 0x54, 0x01, 0x8A, 0x00, 0x05, 0x00, 0x02, 0x00
};
static unsigned char rf_open[] = {
  0x00, 0x00, 0x00, 0x00, 0x04, 0x20, 0x02, 0x40, 0x02, 0x40, 0x11, 0x88, 0x91, 0x89, 0x49, 0x92,
  0x49, 0x92, 0x91, 0x89, 0x11, 0x88, 0x02, 0x40, 0x02, 0x40, 0x04, 0x20, 0x00, 0x00, 0x00, 0x00
};

static unsigned char rf_off[] = {
  0x01, 0x00, 0x02, 0x00, 0x04, 0x20, 0x0A, 0x40, 0x12, 0x40, 0x21, 0x88, 0x51, 0x89, 0x89, 0x92,
  0x49, 0x93, 0x91, 0x8A, 0x11, 0x8C, 0x02, 0x48, 0x02, 0x50, 0x04, 0x20, 0x00, 0x40, 0x00, 0x80
};
static unsigned char card [] = {
  0x00, 0x00, 0xFC, 0x3F, 0x02, 0x40, 0x32, 0x48, 0x52, 0x48, 0x92, 0x48, 0x12, 0x49, 0x12, 0x4A,
  0x52, 0x48, 0x92, 0x48, 0x12, 0x49, 0x12, 0x4A, 0x12, 0x4C, 0x02, 0x40, 0xFC, 0x3F, 0x00, 0x00
};

static unsigned char blank [] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

uint8_t AimeKey[6], BanaKey[6];

//TN32MSEC003S
char FW_TN32[] = {'T', 'N', '3', '2', 'M', 'S', 'E', 'C', '0', '0', '3', 'S', ' ', 'F', '/', 'W', ' ', 'V', 'e', 'r', '1', '.', '2'};
char HW_TN32[] = {'T', 'N', '3', '2', 'M', 'S', 'E', 'C', '0', '0', '3', 'S', ' ', 'H', '/', 'W', ' ', 'V', 'e', 'r', '3', '.', '0'};
char BOARD_TN32[] = {'1', '5', '0', '8', '4', 0xFF, 0x10, 0x00, 0x12};

//837-15396
char FW_837[] = {0x94};
char HW_837[] = {'8', '3', '7', '-', '1', '5', '3', '9', '6'};
char BOARD_837[] = {'0', '0', '0', '-', '0', '0', '0', '0', '0', 0xFF, 0x11, 0x40};

bool FWSW;
enum {
  SG_NFC_CMD_GET_FW_VERSION       = 0x30,
  SG_NFC_CMD_GET_HW_VERSION       = 0x32,
  SG_NFC_CMD_RADIO_ON             = 0x40,
  SG_NFC_CMD_RADIO_OFF            = 0x41,
  SG_NFC_CMD_POLL                 = 0x42,
  SG_NFC_CMD_MIFARE_SELECT_TAG    = 0x43,
  SG_NFC_CMD_MIFARE_SET_KEY_BANA  = 0x50,
  SG_NFC_CMD_BANA_AUTHENTICATE    = 0x51,
  SG_NFC_CMD_MIFARE_READ_BLOCK    = 0x52,
  SG_NFC_CMD_MIFARE_SET_KEY_AIME  = 0x54,
  SG_NFC_CMD_AIME_AUTHENTICATE    = 0x55,
  SG_NFC_CMD_TO_UPDATER_MODE      = 0x60,
  SG_NFC_CMD_SEND_HEX_DATA        = 0x61,
  SG_NFC_CMD_RESET                = 0x62,
  SG_NFC_CMD_FELICA_ENCAP         = 0x71,
  SG_RGB_CMD_SET_COLOR            = 0x81,
  SG_RGB_CMD_GET_INFO             = 0xF0,
  SG_RGB_CMD_RESET                = 0xF5,

  //FELICA_ENCAP
  FELICA_CMD_POLL                 = 0x00,
  FELICA_CMD_NDA_06               = 0x06,
  FELICA_CMD_NDA_08               = 0x08,
  FELICA_CMD_GET_SYSTEM_CODE      = 0x0C,
  FELICA_CMD_NDA_A4               = 0xA4,
};

typedef union packet_req {
  uint8_t bytes[128];
  struct {
    uint8_t frame_len;
    uint8_t addr;
    uint8_t seq_no;
    uint8_t cmd;
    uint8_t payload_len;
    union {
      uint8_t key[6]; //sg_nfc_req_mifare_set_key(bana or aime)
      uint8_t color_payload[3];//sg_led_req_set_color
      struct { //sg_nfc_cmd_mifare_select_tag,sg_nfc_cmd_mifare_authenticate,sg_nfc_cmd_mifare_read_block
        uint8_t uid[4];
        uint8_t block_no;
      };
      struct { //sg_nfc_req_felica_encap
        uint8_t encap_IDm[8];
        uint8_t encap_len;
        uint8_t encap_code;
        union {
          struct {
            uint8_t poll_systemCode[2];
            uint8_t poll_requestCode;
            uint8_t poll_timeout;
          };
          struct { //NDA_06,NDA_08,NDA_A4
            uint8_t RW_IDm[8];
            uint8_t numService;//and NDA_A4 unknown byte
            uint8_t serviceCodeList[2];
            uint8_t numBlock;
            uint8_t blockList[1][2];
            uint8_t blockData[16];//WriteWithoutEncryption,ignore
          };
          uint8_t felica_payload[1];
        };
      };
    };
  };
} packet_req_t;

typedef union packet_res {
  uint8_t bytes[128];
  struct {
    uint8_t frame_len;
    uint8_t addr;
    uint8_t seq_no;
    uint8_t cmd;
    uint8_t status;
    uint8_t payload_len;
    union {
      char version[1]; //sg_nfc_res_get_fw_version,sg_nfc_res_get_hw_version,sg_led_res_get_info
      uint8_t block[16]; //sg_nfc_res_mifare_read_block
      struct { //sg_nfc_res_poll
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
      struct { //sg_nfc_res_felica_encap
        uint8_t encap_len;
        uint8_t encap_code;
        uint8_t encap_IDm[8];
        union {
          struct {//FELICA_CMD_POLL
            uint8_t poll_PMm[8];
            uint8_t poll_systemCode[2];
          };
          struct {
            uint8_t RW_status[2];//??????,NDA_06,NDA_08
            uint8_t numBlock;//NDA_06
            uint8_t blockData[1][1][16];//NDA_06
          };
          uint8_t felica_payload[1];
        };
      };
    };
  };
} packet_res_t;

static packet_req_t req;
static packet_res_t res;

static void sg_res_init(uint8_t payload_len = 0) { //???????????????
  res.frame_len = 6 + payload_len;
  res.addr = req.addr;
  res.seq_no = req.seq_no;
  res.cmd = req.cmd;
  res.status = 0;
  res.payload_len = payload_len;
}

static void sg_nfc_cmd_reset() { //???????????????
  FWSW = !digitalRead(SW2);
  nfc.begin();
  nfc.setPassiveActivationRetries(0x10); //??????????????????,0xFF????????????
  nfc.SAMConfig();
  if (nfc.getFirmwareVersion()) {
    nfc.SAMConfig();
    sg_res_init();
    res.status = 3;
    u8g2.drawXBM(0, 0, 16, 16, usb_connect);
    u8g2.drawXBM(20, 0, 16, 16, blank);
    u8g2.drawXBM(111, 0, 16, 16, blank);
    u8g2.sendBuffer();
    return;
  }
  u8g2.drawXBM(20, 0, 16, 16, rf_off);
  u8g2.sendBuffer();
  FastLED.showColor(0xFF0000);
}

static void sg_nfc_cmd_get_fw_version() {
  if (FWSW) {
    sg_res_init(sizeof(FW_TN32));
    memcpy(res.version, FW_TN32, res.payload_len);
  } else {
    sg_res_init(sizeof(FW_837));
    memcpy(res.version, FW_837, res.payload_len);
  }
}

static void sg_nfc_cmd_get_hw_version() {
  if (FWSW) {
    sg_res_init(sizeof(HW_TN32));
    memcpy(res.version, HW_TN32, res.payload_len);
  } else {
    sg_res_init(sizeof(HW_837));
    memcpy(res.version, HW_837, res.payload_len);
  }
}

static void sg_led_cmd_get_info() {
  if (FWSW) {
    sg_res_init(sizeof(BOARD_TN32));
    memcpy(res.version, BOARD_TN32, res.payload_len);
  } else {
    sg_res_init(sizeof(BOARD_837));
    memcpy(res.version, BOARD_837, res.payload_len);
  }
}

static void sg_nfc_cmd_mifare_set_key_aime() {
  sg_res_init();
  memcpy(AimeKey, req.key, 6);
}

static void sg_nfc_cmd_mifare_set_key_bana() {
  sg_res_init();
  memcpy(BanaKey, req.key, 6);
}

static void sg_led_cmd_reset() {
  sg_res_init();
}

static void sg_led_cmd_set_color() {
  FastLED.showColor(CRGB(req.color_payload[0], req.color_payload[1], req.color_payload[2]));
}

static void sg_nfc_cmd_radio_on() {
  sg_res_init();
  nfc.setRFField(0x00, 0x01);
  u8g2.drawXBM(20, 0, 16, 16, rf_open);
  u8g2.sendBuffer();
}

static void sg_nfc_cmd_radio_off() {
  sg_res_init();
  nfc.setRFField(0x00, 0x00);
  u8g2.drawXBM(20, 0, 16, 16, blank);
  u8g2.sendBuffer();
}

static void sg_nfc_cmd_poll() { //????????????
  uint16_t SystemCode;
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, res.mifare_uid, &res.id_len)) {
    sg_res_init(0x07);
    res.count = 1;
    res.type = 0x10;
    u8g2.drawXBM(111, 0, 16, 16, card);
    u8g2.sendBuffer();
    return;
  }
  else if (nfc.felica_Polling(0xFFFF, 0x00, res.IDm, res.PMm, &SystemCode, 200) == 1) {//< 0: error
    sg_res_init(0x13);
    res.count = 1;
    res.type = 0x20;
    res.id_len = 0x10;
    u8g2.drawXBM(111, 0, 16, 16, card);
    u8g2.sendBuffer();
    return;
  } else {
    sg_res_init(1);
    res.count = 0;
    u8g2.drawXBM(111, 0, 16, 16, blank);
    u8g2.sendBuffer();
  }
}

static void sg_nfc_cmd_mifare_select_tag() {
  sg_res_init();
}

static void sg_nfc_cmd_aime_authenticate() {
  sg_res_init();
  if (nfc.mifareclassic_AuthenticateBlock(req.uid, 4, req.block_no, 1, AimeKey)) {
    return;
  } else {
    res.status = 1;
  }
}

static void sg_nfc_cmd_bana_authenticate() {
  sg_res_init();
  if (nfc.mifareclassic_AuthenticateBlock(req.uid, 4, req.block_no, 0, BanaKey)) {
    return;
  } else {
    res.status = 1;
  }
}

static void sg_nfc_cmd_mifare_read_block() {//?????????????????????
  if (nfc.mifareclassic_ReadDataBlock(req.block_no, res.block)) {
    sg_res_init(0x10);
    return;
  }
  sg_res_init();
  res.status = 1;
}

static void sg_nfc_cmd_felica_encap() {
  uint16_t SystemCode;
  if (nfc.felica_Polling(0xFFFF, 0x01, res.encap_IDm, res.poll_PMm, &SystemCode, 200) == 1) {
    SystemCode = SystemCode >> 8 | SystemCode << 8;//SystemCode????????????????????????
  }
  else {
    sg_res_init();
    res.status = 1;
    return;
  }
  uint8_t code = req.encap_code;
  res.encap_code = code + 1;
  switch (code) {
    case FELICA_CMD_POLL:
      {
        sg_res_init(0x14);
        res.poll_systemCode[0] = SystemCode;
        res.poll_systemCode[1] = SystemCode >> 8;
      }
      break;
    case FELICA_CMD_GET_SYSTEM_CODE:
      {
        sg_res_init(0x0D);
        res.felica_payload[0] = 0x01;//??????
        res.felica_payload[1] = SystemCode;//SystemCode
        res.felica_payload[2] = SystemCode >> 8;
      }
      break;
    case FELICA_CMD_NDA_A4:
      {
        sg_res_init(0x0B);
        res.felica_payload[0] = 0x00;
      }
      break;
    case FELICA_CMD_NDA_06:
      {
        uint16_t serviceCodeList[1] = {(uint16_t)(req.serviceCodeList[1] << 8 | req.serviceCodeList[0])};//?????????????????????
        for (uint8_t i = 0; i < req.numBlock; i++) {
          uint16_t blockList[1] = {(uint16_t)(req.blockList[i][0] << 8 | req.blockList[i][1])};
          if (nfc.felica_ReadWithoutEncryption(1, serviceCodeList, 1, blockList, res.blockData[i]) != 1) {
            memset(res.blockData[i], 0, 16);//dummy data
          }
        }
        res.RW_status[0] = 0;
        res.RW_status[1] = 0;
        res.numBlock = req.numBlock;
        sg_res_init(0x0D + req.numBlock * 16);
      }
      break;
    case FELICA_CMD_NDA_08:
      {
        sg_res_init(0x0C);//?????????????????????????????????????????????
        res.RW_status[0] = 0;
        res.RW_status[1] = 0;
      }
      break;
    default:
      sg_res_init();
      res.status = 1;
  }
  res.encap_len = res.payload_len;
}
