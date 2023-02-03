/*
库代码来自：https://github.com/elechouse/PN532
删除了未使用的函数。
*/
/**************************************************************************/
/*!
    @file     PN532.h
    @author   Adafruit Industries & Seeed Studio
    @license  BSD
*/
/**************************************************************************/

#ifndef __PN532_H__
#define __PN532_H__

#include "PN532Interface.h"

// PN532 Commands
#define PN532_COMMAND_DIAGNOSE              (0x00)
#define PN532_COMMAND_GETFIRMWAREVERSION    (0x02)
#define PN532_COMMAND_GETGENERALSTATUS      (0x04)
#define PN532_COMMAND_READREGISTER          (0x06)
#define PN532_COMMAND_WRITEREGISTER         (0x08)
#define PN532_COMMAND_READGPIO              (0x0C)
#define PN532_COMMAND_WRITEGPIO             (0x0E)
#define PN532_COMMAND_SETSERIALBAUDRATE     (0x10)
#define PN532_COMMAND_SETPARAMETERS         (0x12)
#define PN532_COMMAND_SAMCONFIGURATION      (0x14)
#define PN532_COMMAND_POWERDOWN             (0x16)
#define PN532_COMMAND_RFCONFIGURATION       (0x32)
#define PN532_COMMAND_RFREGULATIONTEST      (0x58)
#define PN532_COMMAND_INJUMPFORDEP          (0x56)
#define PN532_COMMAND_INJUMPFORPSL          (0x46)
#define PN532_COMMAND_INLISTPASSIVETARGET   (0x4A)
#define PN532_COMMAND_INATR                 (0x50)
#define PN532_COMMAND_INPSL                 (0x4E)
#define PN532_COMMAND_INDATAEXCHANGE        (0x40)
#define PN532_COMMAND_INCOMMUNICATETHRU     (0x42)
#define PN532_COMMAND_INDESELECT            (0x44)
#define PN532_COMMAND_INRELEASE             (0x52)
#define PN532_COMMAND_INSELECT              (0x54)
#define PN532_COMMAND_INAUTOPOLL            (0x60)
#define PN532_COMMAND_TGINITASTARGET        (0x8C)
#define PN532_COMMAND_TGSETGENERALBYTES     (0x92)
#define PN532_COMMAND_TGGETDATA             (0x86)
#define PN532_COMMAND_TGSETDATA             (0x8E)
#define PN532_COMMAND_TGSETMETADATA         (0x94)
#define PN532_COMMAND_TGGETINITIATORCOMMAND (0x88)
#define PN532_COMMAND_TGRESPONSETOINITIATOR (0x90)
#define PN532_COMMAND_TGGETTARGETSTATUS     (0x8A)

#define PN532_RESPONSE_INDATAEXCHANGE       (0x41)
#define PN532_RESPONSE_INLISTPASSIVETARGET  (0x4B)


#define PN532_MIFARE_ISO14443A              (0x00)

// Mifare Commands
#define MIFARE_CMD_AUTH_A                   (0x60)
#define MIFARE_CMD_AUTH_B                   (0x61)
#define MIFARE_CMD_READ                     (0x30)
#define MIFARE_CMD_WRITE                    (0xA0)
#define MIFARE_CMD_WRITE_ULTRALIGHT         (0xA2)
#define MIFARE_CMD_TRANSFER                 (0xB0)
#define MIFARE_CMD_DECREMENT                (0xC0)
#define MIFARE_CMD_INCREMENT                (0xC1)
#define MIFARE_CMD_STORE                    (0xC2)

// FeliCa Commands
#define FELICA_CMD_POLLING                  (0x00)
#define FELICA_CMD_REQUEST_SERVICE          (0x02)
#define FELICA_CMD_REQUEST_RESPONSE         (0x04)
#define FELICA_CMD_READ_WITHOUT_ENCRYPTION  (0x06)
#define FELICA_CMD_WRITE_WITHOUT_ENCRYPTION (0x08)
#define FELICA_CMD_REQUEST_SYSTEM_CODE      (0x0C)

// FeliCa consts
#define FELICA_READ_MAX_SERVICE_NUM         16
#define FELICA_READ_MAX_BLOCK_NUM           12 // for typical FeliCa card
#define FELICA_WRITE_MAX_SERVICE_NUM        16
#define FELICA_WRITE_MAX_BLOCK_NUM          10 // for typical FeliCa card
#define FELICA_REQ_SERVICE_MAX_NODE_NUM     32

class PN532
{
public:
    PN532(PN532Interface &interface);

    void begin(void);

    // Generic PN532 functions
    bool SAMConfig(void);
    uint32_t getFirmwareVersion(void);
    uint32_t readRegister(uint16_t reg);
    uint32_t writeRegister(uint16_t reg, uint8_t val);
    bool setPassiveActivationRetries(uint8_t maxRetries);
    bool setRFField(uint8_t autoRFCA, uint8_t rFOnOff);

    /**
    * @brief    Init PN532 as a target
    * @param    timeout max time to wait, 0 means no timeout
    * @return   > 0     success
    *           = 0     timeout
    *           < 0     failed
    */
    int8_t tgInitAsTarget(uint16_t timeout = 0);
    int8_t tgInitAsTarget(const uint8_t* command, const uint8_t len, const uint16_t timeout = 0);

    int16_t tgGetData(uint8_t *buf, uint8_t len);
    bool tgSetData(const uint8_t *header, uint8_t hlen, const uint8_t *body = 0, uint8_t blen = 0);

    int16_t inRelease(const uint8_t relevantTarget = 0);

    // ISO14443A functions
    bool inListPassiveTarget();
    bool readPassiveTargetID(uint8_t cardbaudrate, uint8_t *uid, uint8_t *uidLength, uint16_t timeout = 1000);
    bool inDataExchange(uint8_t *send, uint8_t sendLength, uint8_t *response, uint8_t *responseLength);

    // Mifare Classic functions
    bool mifareclassic_IsFirstBlock (uint32_t uiBlock);
    bool mifareclassic_IsTrailerBlock (uint32_t uiBlock);
    uint8_t mifareclassic_AuthenticateBlock (uint8_t *uid, uint8_t uidLen, uint32_t blockNumber, uint8_t keyNumber, uint8_t *keyData);
    uint8_t mifareclassic_ReadDataBlock (uint8_t blockNumber, uint8_t *data);
    uint8_t mifareclassic_WriteDataBlock (uint8_t blockNumber, uint8_t *data);

    // FeliCa Functions
    int8_t felica_Polling(uint16_t systemCode, uint8_t requestCode, uint8_t *idm, uint8_t *pmm, uint16_t *systemCodeResponse, uint16_t timeout=1000);
    int8_t felica_SendCommand (const uint8_t * command, uint8_t commandlength, uint8_t * response, uint8_t * responseLength);
    int8_t felica_RequestService(uint8_t numNode, uint16_t *nodeCodeList, uint16_t *keyVersions) ;
    int8_t felica_RequestResponse(uint8_t *mode);
    int8_t felica_ReadWithoutEncryption (uint8_t numService, const uint16_t *serviceCodeList, uint8_t numBlock, const uint16_t *blockList, uint8_t blockData[][16]);
    int8_t felica_WriteWithoutEncryption (uint8_t numService, const uint16_t *serviceCodeList, uint8_t numBlock, const uint16_t *blockList, uint8_t blockData[][16]);
    int8_t felica_RequestSystemCode(uint8_t *numSystemCode, uint16_t *systemCodeList);
    int8_t felica_Release();

    uint8_t *getBuffer(uint8_t *len) {
        *len = sizeof(pn532_packetbuffer) - 4;
        return pn532_packetbuffer;
    };

private:
    uint8_t _uid[7];  // ISO14443A uid
    uint8_t _uidLen;  // uid len
    uint8_t _key[6];  // Mifare Classic key
    uint8_t inListedTag; // Tg number of inlisted tag.
    uint8_t _felicaIDm[8]; // FeliCa IDm (NFCID2)
    uint8_t _felicaPMm[8]; // FeliCa PMm (PAD)

    uint8_t pn532_packetbuffer[64];

    PN532Interface *_interface;
};

#endif
