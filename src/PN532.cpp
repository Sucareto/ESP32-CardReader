/*
库代码来自：https://github.com/elechouse/PN532
删除了未使用的函数。
*/
/**************************************************************************/
/*!
    @file     PN532.cpp
    @author   Adafruit Industries & Seeed Studio
    @license  BSD
*/
/**************************************************************************/

#include "Arduino.h"
#include "PN532.h"

#define HAL(func) (_interface->func)

PN532::PN532(PN532Interface &interface) {
  _interface = &interface;
}

/**************************************************************************/
/*!
    @brief  Setups the HW
*/
/**************************************************************************/
void PN532::begin() {
  HAL(begin)
  ();
  HAL(wakeup)
  ();
}

/**************************************************************************/
/*!
    @brief  Checks the firmware version of the PN5xx chip

    @returns  The chip's firmware version and ID
*/
/**************************************************************************/
uint32_t PN532::getFirmwareVersion(void) {
  uint32_t response;

  pn532_packetbuffer[0] = PN532_COMMAND_GETFIRMWAREVERSION;

  if (HAL(writeCommand)(pn532_packetbuffer, 1)) {
    return 0;
  }

  // read data packet
  int16_t status = HAL(readResponse)(pn532_packetbuffer, sizeof(pn532_packetbuffer));
  if (0 > status) {
    return 0;
  }

  response = pn532_packetbuffer[0];
  response <<= 8;
  response |= pn532_packetbuffer[1];
  response <<= 8;
  response |= pn532_packetbuffer[2];
  response <<= 8;
  response |= pn532_packetbuffer[3];

  return response;
}


/**************************************************************************/
/*!
    @brief  Read a PN532 register.

    @param  reg  the 16-bit register address.

    @returns  The register value.
*/
/**************************************************************************/
uint32_t PN532::readRegister(uint16_t reg) {
  uint32_t response;

  pn532_packetbuffer[0] = PN532_COMMAND_READREGISTER;
  pn532_packetbuffer[1] = (reg >> 8) & 0xFF;
  pn532_packetbuffer[2] = reg & 0xFF;

  if (HAL(writeCommand)(pn532_packetbuffer, 3)) {
    return 0;
  }

  // read data packet
  int16_t status = HAL(readResponse)(pn532_packetbuffer, sizeof(pn532_packetbuffer));
  if (0 > status) {
    return 0;
  }

  response = pn532_packetbuffer[0];

  return response;
}

/**************************************************************************/
/*!
    @brief  Write to a PN532 register.

    @param  reg  the 16-bit register address.
    @param  val  the 8-bit value to write.

    @returns  0 for failure, 1 for success.
*/
/**************************************************************************/
uint32_t PN532::writeRegister(uint16_t reg, uint8_t val) {
  uint32_t response;

  pn532_packetbuffer[0] = PN532_COMMAND_WRITEREGISTER;
  pn532_packetbuffer[1] = (reg >> 8) & 0xFF;
  pn532_packetbuffer[2] = reg & 0xFF;
  pn532_packetbuffer[3] = val;


  if (HAL(writeCommand)(pn532_packetbuffer, 4)) {
    return 0;
  }

  // read data packet
  int16_t status = HAL(readResponse)(pn532_packetbuffer, sizeof(pn532_packetbuffer));
  if (0 > status) {
    return 0;
  }

  return 1;
}

/**************************************************************************/
/*!
    @brief  Configures the SAM (Secure Access Module)
*/
/**************************************************************************/
bool PN532::SAMConfig(void) {
  pn532_packetbuffer[0] = PN532_COMMAND_SAMCONFIGURATION;
  pn532_packetbuffer[1] = 0x01;  // normal mode;
  pn532_packetbuffer[2] = 0x14;  // timeout 50ms * 20 = 1 second
  pn532_packetbuffer[3] = 0x01;  // use IRQ pin!

  if (HAL(writeCommand)(pn532_packetbuffer, 4))
    return false;

  return (0 < HAL(readResponse)(pn532_packetbuffer, sizeof(pn532_packetbuffer)));
}

/**************************************************************************/
/*!
    Sets the MxRtyPassiveActivation uint8_t of the RFConfiguration register

    @param  maxRetries    0xFF to wait forever, 0x00..0xFE to timeout
                          after mxRetries

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
bool PN532::setPassiveActivationRetries(uint8_t maxRetries) {
  pn532_packetbuffer[0] = PN532_COMMAND_RFCONFIGURATION;
  pn532_packetbuffer[1] = 5;     // Config item 5 (MaxRetries)
  pn532_packetbuffer[2] = 0xFF;  // MxRtyATR (default = 0xFF)
  pn532_packetbuffer[3] = 0x01;  // MxRtyPSL (default = 0x01)
  pn532_packetbuffer[4] = maxRetries;

  if (HAL(writeCommand)(pn532_packetbuffer, 5))
    return 0x0;  // no ACK

  return (0 < HAL(readResponse)(pn532_packetbuffer, sizeof(pn532_packetbuffer)));
}

/**************************************************************************/
/*!
    Sets the RFon/off uint8_t of the RFConfiguration register

    @param  autoRFCA    0x00 No check of the external field before 
                        activation 
                        
                        0x02 Check the external field before 
                        activation

    @param  rFOnOff     0x00 Switch the RF field off, 0x01 switch the RF 
                        field on

    @returns    1 if everything executed properly, 0 for an error
*/
/**************************************************************************/

bool PN532::setRFField(uint8_t autoRFCA, uint8_t rFOnOff) {
  pn532_packetbuffer[0] = PN532_COMMAND_RFCONFIGURATION;
  pn532_packetbuffer[1] = 1;
  pn532_packetbuffer[2] = 0x00 | autoRFCA | rFOnOff;

  if (HAL(writeCommand)(pn532_packetbuffer, 3)) {
    return 0x0;  // command failed
  }

  return (0 < HAL(readResponse)(pn532_packetbuffer, sizeof(pn532_packetbuffer)));
}

/***** ISO14443A Commands ******/

/**************************************************************************/
/*!
    Waits for an ISO14443A target to enter the field

    @param  cardBaudRate  Baud rate of the card
    @param  uid           Pointer to the array that will be populated
                          with the card's UID (up to 7 bytes)
    @param  uidLength     Pointer to the variable that will hold the
                          length of the card's UID.

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
bool PN532::readPassiveTargetID(uint8_t cardbaudrate, uint8_t *uid, uint8_t *uidLength, uint16_t timeout) {
  pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
  pn532_packetbuffer[1] = 1;  // max 1 cards at once (we can set this to 2 later)
  pn532_packetbuffer[2] = cardbaudrate;

  if (HAL(writeCommand)(pn532_packetbuffer, 3)) {
    return 0x0;  // command failed
  }

  // read data packet
  if (HAL(readResponse)(pn532_packetbuffer, sizeof(pn532_packetbuffer), timeout) < 0) {
    return 0x0;
  }

  // check some basic stuff
  /* ISO14443A card response should be in the following format:

      byte            Description
      -------------   ------------------------------------------
      b0              Tags Found
      b1              Tag Number (only one used in this example)
      b2..3           SENS_RES
      b4              SEL_RES
      b5              NFCID Length
      b6..NFCIDLen    NFCID
    */

  if (pn532_packetbuffer[0] != 1)
    return 0;

  if (pn532_packetbuffer[4] != 0x08) // SAK == 0x08
    return 0;

  uint16_t sens_res = pn532_packetbuffer[2];
  sens_res <<= 8;
  sens_res |= pn532_packetbuffer[3];

  /* Card appears to be Mifare Classic */
  *uidLength = pn532_packetbuffer[5];

  for (uint8_t i = 0; i < pn532_packetbuffer[5]; i++) {
    uid[i] = pn532_packetbuffer[6 + i];
  }

  return 1;
}


/***** Mifare Classic Functions ******/

/**************************************************************************/
/*!
      Indicates whether the specified block number is the first block
      in the sector (block 0 relative to the current sector)
*/
/**************************************************************************/
bool PN532::mifareclassic_IsFirstBlock(uint32_t uiBlock) {
  // Test if we are in the small or big sectors
  if (uiBlock < 128)
    return ((uiBlock) % 4 == 0);
  else
    return ((uiBlock) % 16 == 0);
}

/**************************************************************************/
/*!
      Indicates whether the specified block number is the sector trailer
*/
/**************************************************************************/
bool PN532::mifareclassic_IsTrailerBlock(uint32_t uiBlock) {
  // Test if we are in the small or big sectors
  if (uiBlock < 128)
    return ((uiBlock + 1) % 4 == 0);
  else
    return ((uiBlock + 1) % 16 == 0);
}

/**************************************************************************/
/*!
    Tries to authenticate a block of memory on a MIFARE card using the
    INDATAEXCHANGE command.  See section 7.3.8 of the PN532 User Manual
    for more information on sending MIFARE and other commands.

    @param  uid           Pointer to a byte array containing the card UID
    @param  uidLen        The length (in bytes) of the card's UID (Should
                          be 4 for MIFARE Classic)
    @param  blockNumber   The block number to authenticate.  (0..63 for
                          1KB cards, and 0..255 for 4KB cards).
    @param  keyNumber     Which key type to use during authentication
                          (0 = MIFARE_CMD_AUTH_A, 1 = MIFARE_CMD_AUTH_B)
    @param  keyData       Pointer to a byte array containing the 6 bytes
                          key value

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t PN532::mifareclassic_AuthenticateBlock(uint8_t *uid, uint8_t uidLen, uint32_t blockNumber, uint8_t keyNumber, uint8_t *keyData) {
  uint8_t i;

  // Hang on to the key and uid data
  memcpy(_key, keyData, 6);
  memcpy(_uid, uid, uidLen);
  _uidLen = uidLen;

  // Prepare the authentication command //
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE; /* Data Exchange Header */
  pn532_packetbuffer[1] = 1;                            /* Max card numbers */
  pn532_packetbuffer[2] = (keyNumber) ? MIFARE_CMD_AUTH_B : MIFARE_CMD_AUTH_A;
  pn532_packetbuffer[3] = blockNumber; /* Block Number (1K = 0..63, 4K = 0..255 */
  memcpy(pn532_packetbuffer + 4, _key, 6);
  for (i = 0; i < _uidLen; i++) {
    pn532_packetbuffer[10 + i] = _uid[i]; /* 4 bytes card ID */
  }

  if (HAL(writeCommand)(pn532_packetbuffer, 10 + _uidLen))
    return 0;

  // Read the response packet
  HAL(readResponse)
  (pn532_packetbuffer, sizeof(pn532_packetbuffer));

  // Check if the response is valid and we are authenticated???
  // for an auth success it should be bytes 5-7: 0xD5 0x41 0x00
  // Mifare auth error is technically byte 7: 0x14 but anything other and 0x00 is not good
  if (pn532_packetbuffer[0] != 0x00) {
    return 0;
  }

  return 1;
}

/**************************************************************************/
/*!
    Tries to read an entire 16-bytes data block at the specified block
    address.

    @param  blockNumber   The block number to authenticate.  (0..63 for
                          1KB cards, and 0..255 for 4KB cards).
    @param  data          Pointer to the byte array that will hold the
                          retrieved data (if any)

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t PN532::mifareclassic_ReadDataBlock(uint8_t blockNumber, uint8_t *data) {

  /* Prepare the command */
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1;               /* Card number */
  pn532_packetbuffer[2] = MIFARE_CMD_READ; /* Mifare Read command = 0x30 */
  pn532_packetbuffer[3] = blockNumber;     /* Block Number (0..63 for 1K, 0..255 for 4K) */

  /* Send the command */
  if (HAL(writeCommand)(pn532_packetbuffer, 4)) {
    return 0;
  }

  /* Read the response packet */
  HAL(readResponse)
  (pn532_packetbuffer, sizeof(pn532_packetbuffer));

  /* If byte 8 isn't 0x00 we probably have an error */
  if (pn532_packetbuffer[0] != 0x00) {
    return 0;
  }

  /* Copy the 16 data bytes to the output buffer        */
  /* Block content starts at byte 9 of a valid response */
  memcpy(data, pn532_packetbuffer + 1, 16);

  return 1;
}

/**************************************************************************/
/*!
    Tries to write an entire 16-bytes data block at the specified block
    address.

    @param  blockNumber   The block number to authenticate.  (0..63 for
                          1KB cards, and 0..255 for 4KB cards).
    @param  data          The byte array that contains the data to write.

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t PN532::mifareclassic_WriteDataBlock(uint8_t blockNumber, uint8_t *data) {
  /* Prepare the first command */
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1;                /* Card number */
  pn532_packetbuffer[2] = MIFARE_CMD_WRITE; /* Mifare Write command = 0xA0 */
  pn532_packetbuffer[3] = blockNumber;      /* Block Number (0..63 for 1K, 0..255 for 4K) */
  memcpy(pn532_packetbuffer + 4, data, 16); /* Data Payload */

  /* Send the command */
  if (HAL(writeCommand)(pn532_packetbuffer, 20)) {
    return 0;
  }

  /* Read the response packet */
  return (0 < HAL(readResponse)(pn532_packetbuffer, sizeof(pn532_packetbuffer)));
}

/**************************************************************************/
/*!
    @brief  Exchanges an APDU with the currently inlisted peer

    @param  send            Pointer to data to send
    @param  sendLength      Length of the data to send
    @param  response        Pointer to response data
    @param  responseLength  Pointer to the response data length
*/
/**************************************************************************/
bool PN532::inDataExchange(uint8_t *send, uint8_t sendLength, uint8_t *response, uint8_t *responseLength) {
  uint8_t i;

  pn532_packetbuffer[0] = 0x40;  // PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = inListedTag;

  if (HAL(writeCommand)(pn532_packetbuffer, 2, send, sendLength)) {
    return false;
  }

  int16_t status = HAL(readResponse)(response, *responseLength, 1000);
  if (status < 0) {
    return false;
  }

  if ((response[0] & 0x3f) != 0) {
    return false;
  }

  uint8_t length = status;
  length -= 1;

  if (length > *responseLength) {
    length = *responseLength;  // silent truncation...
  }

  for (uint8_t i = 0; i < length; i++) {
    response[i] = response[i + 1];
  }
  *responseLength = length;

  return true;
}

/**************************************************************************/
/*!
    @brief  'InLists' a passive target. PN532 acting as reader/initiator,
            peer acting as card/responder.
*/
/**************************************************************************/
bool PN532::inListPassiveTarget() {
  pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
  pn532_packetbuffer[1] = 1;
  pn532_packetbuffer[2] = 0;

  if (HAL(writeCommand)(pn532_packetbuffer, 3)) {
    return false;
  }

  int16_t status = HAL(readResponse)(pn532_packetbuffer, sizeof(pn532_packetbuffer), 30000);
  if (status < 0) {
    return false;
  }

  if (pn532_packetbuffer[0] != 1) {
    return false;
  }

  inListedTag = pn532_packetbuffer[1];

  return true;
}

int8_t PN532::tgInitAsTarget(const uint8_t *command, const uint8_t len, const uint16_t timeout) {

  int8_t status = HAL(writeCommand)(command, len);
  if (status < 0) {
    return -1;
  }

  status = HAL(readResponse)(pn532_packetbuffer, sizeof(pn532_packetbuffer), timeout);
  if (status > 0) {
    return 1;
  } else if (PN532_TIMEOUT == status) {
    return 0;
  } else {
    return -2;
  }
}

/**
 * Peer to Peer
 */
int8_t PN532::tgInitAsTarget(uint16_t timeout) {
  const uint8_t command[] = {
    PN532_COMMAND_TGINITASTARGET,
    0,
    0x00, 0x00,        //SENS_RES
    0x00, 0x00, 0x00,  //NFCID1
    0x40,              //SEL_RES

    0x01, 0xFE, 0x0F, 0xBB, 0xBA, 0xA6, 0xC9, 0x89,  // POL_RES
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF,

    0x01, 0xFE, 0x0F, 0xBB, 0xBA, 0xA6, 0xC9, 0x89, 0x00, 0x00,  //NFCID3t: Change this to desired value

    0x06, 0x46, 0x66, 0x6D, 0x01, 0x01, 0x10, 0x00  // LLCP magic number and version parameter
  };
  return tgInitAsTarget(command, sizeof(command), timeout);
}

int16_t PN532::tgGetData(uint8_t *buf, uint8_t len) {
  buf[0] = PN532_COMMAND_TGGETDATA;

  if (HAL(writeCommand)(buf, 1)) {
    return -1;
  }

  int16_t status = HAL(readResponse)(buf, len, 3000);
  if (0 >= status) {
    return status;
  }

  uint16_t length = status - 1;


  if (buf[0] != 0) {
    return -5;
  }

  for (uint8_t i = 0; i < length; i++) {
    buf[i] = buf[i + 1];
  }

  return length;
}

bool PN532::tgSetData(const uint8_t *header, uint8_t hlen, const uint8_t *body, uint8_t blen) {
  if (hlen > (sizeof(pn532_packetbuffer) - 1)) {
    if ((body != 0) || (header == pn532_packetbuffer)) {
      return false;
    }

    pn532_packetbuffer[0] = PN532_COMMAND_TGSETDATA;
    if (HAL(writeCommand)(pn532_packetbuffer, 1, header, hlen)) {
      return false;
    }
  } else {
    for (int8_t i = hlen - 1; i >= 0; i--) {
      pn532_packetbuffer[i + 1] = header[i];
    }
    pn532_packetbuffer[0] = PN532_COMMAND_TGSETDATA;

    if (HAL(writeCommand)(pn532_packetbuffer, hlen + 1, body, blen)) {
      return false;
    }
  }

  if (0 > HAL(readResponse)(pn532_packetbuffer, sizeof(pn532_packetbuffer), 3000)) {
    return false;
  }

  if (0 != pn532_packetbuffer[0]) {
    return false;
  }

  return true;
}

int16_t PN532::inRelease(const uint8_t relevantTarget) {

  pn532_packetbuffer[0] = PN532_COMMAND_INRELEASE;
  pn532_packetbuffer[1] = relevantTarget;

  if (HAL(writeCommand)(pn532_packetbuffer, 2)) {
    return 0;
  }

  // read data packet
  return HAL(readResponse)(pn532_packetbuffer, sizeof(pn532_packetbuffer));
}


/***** FeliCa Functions ******/
/**************************************************************************/
/*!
    @brief  Poll FeliCa card. PN532 acting as reader/initiator,
            peer acting as card/responder.
    @param[in]  systemCode             Designation of System Code. When sending FFFFh as System Code,
                                       all FeliCa cards can return response.
    @param[in]  requestCode            Designation of Request Data as follows:
                                         00h: No Request
                                         01h: System Code request (to acquire System Code of the card)
                                         02h: Communication perfomance request
    @param[out] idm                    IDm of the card (8 bytes)
    @param[out] pmm                    PMm of the card (8 bytes)
    @param[out] systemCodeResponse     System Code of the card (Optional, 2bytes)
    @return                            = 1: A FeliCa card has detected
                                       = 0: No card has detected
                                       < 0: error
*/
/**************************************************************************/
int8_t PN532::felica_Polling(uint16_t systemCode, uint8_t requestCode, uint8_t *idm, uint8_t *pmm, uint16_t *systemCodeResponse, uint16_t timeout) {
  pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
  pn532_packetbuffer[1] = 1;
  pn532_packetbuffer[2] = 1;
  pn532_packetbuffer[3] = FELICA_CMD_POLLING;
  pn532_packetbuffer[4] = (systemCode >> 8) & 0xFF;
  pn532_packetbuffer[5] = systemCode & 0xFF;
  pn532_packetbuffer[6] = requestCode;
  pn532_packetbuffer[7] = 0;

  if (HAL(writeCommand)(pn532_packetbuffer, 8)) {
    return -1;
  }

  int16_t status = HAL(readResponse)(pn532_packetbuffer, 22, timeout);
  if (status < 0) {
    return -2;
  }

  // Check NbTg (pn532_packetbuffer[7])
  if (pn532_packetbuffer[0] == 0) {
    return 0;
  } else if (pn532_packetbuffer[0] != 1) {
    return -3;
  }

  inListedTag = pn532_packetbuffer[1];

  // length check
  uint8_t responseLength = pn532_packetbuffer[2];
  if (responseLength != 18 && responseLength != 20) {
    return -4;
  }

  uint8_t i;
  for (i = 0; i < 8; ++i) {
    idm[i] = pn532_packetbuffer[4 + i];
    _felicaIDm[i] = pn532_packetbuffer[4 + i];
    pmm[i] = pn532_packetbuffer[12 + i];
    _felicaPMm[i] = pn532_packetbuffer[12 + i];
  }

  if (responseLength == 20) {
    *systemCodeResponse = (uint16_t)((pn532_packetbuffer[20] << 8) + pn532_packetbuffer[21]);
  }

  return 1;
}

/**************************************************************************/
/*!
    @brief  Sends FeliCa command to the currently inlisted peer

    @param[in]  command         FeliCa command packet. (e.g. 00 FF FF 00 00  for Polling command)
    @param[in]  commandlength   Length of the FeliCa command packet. (e.g. 0x05 for above Polling command )
    @param[out] response        FeliCa response packet. (e.g. 01 NFCID2(8 bytes) PAD(8 bytes)  for Polling response)
    @param[out] responselength  Length of the FeliCa response packet. (e.g. 0x11 for above Polling command )
    @return                          = 1: Success
                                     < 0: error
*/
/**************************************************************************/
int8_t PN532::felica_SendCommand(const uint8_t *command, uint8_t commandlength, uint8_t *response, uint8_t *responseLength) {
  if (commandlength > 0xFE) {
    return -1;
  }

  pn532_packetbuffer[0] = 0x40;  // PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = inListedTag;
  pn532_packetbuffer[2] = commandlength + 1;

  if (HAL(writeCommand)(pn532_packetbuffer, 3, command, commandlength)) {
    return -2;
  }

  // Wait card response
  int16_t status = HAL(readResponse)(pn532_packetbuffer, sizeof(pn532_packetbuffer), 200);
  if (status < 0) {
    return -3;
  }

  // Check status (pn532_packetbuffer[0])
  if ((pn532_packetbuffer[0] & 0x3F) != 0) {
    return -4;
  }

  // length check
  *responseLength = pn532_packetbuffer[1] - 1;
  if ((status - 2) != *responseLength) {
    return -5;
  }

  memcpy(response, &pn532_packetbuffer[2], *responseLength);

  return 1;
}


/**************************************************************************/
/*!
    @brief  Sends FeliCa Request Service command

    @param[in]  numNode           length of the nodeCodeList
    @param[in]  nodeCodeList      Node codes(Big Endian)
    @param[out] keyVersions       Key Version of each Node (Big Endian)
    @return                          = 1: Success
                                     < 0: error
*/
/**************************************************************************/
int8_t PN532::felica_RequestService(uint8_t numNode, uint16_t *nodeCodeList, uint16_t *keyVersions) {
  if (numNode > FELICA_REQ_SERVICE_MAX_NODE_NUM) {
    return -1;
  }

  uint8_t i, j = 0;
  uint8_t cmdLen = 1 + 8 + 1 + 2 * numNode;
  uint8_t cmd[cmdLen];
  cmd[j++] = FELICA_CMD_REQUEST_SERVICE;
  for (i = 0; i < 8; ++i) {
    cmd[j++] = _felicaIDm[i];
  }
  cmd[j++] = numNode;
  for (i = 0; i < numNode; ++i) {
    cmd[j++] = nodeCodeList[i] & 0xFF;
    cmd[j++] = (nodeCodeList[i] >> 8) & 0xff;
  }

  uint8_t response[10 + 2 * numNode];
  uint8_t responseLength;

  if (felica_SendCommand(cmd, cmdLen, response, &responseLength) != 1) {
    return -2;
  }

  // length check
  if (responseLength != 10 + 2 * numNode) {
    return -3;
  }

  for (i = 0; i < numNode; i++) {
    keyVersions[i] = (uint16_t)(response[10 + i * 2] + (response[10 + i * 2 + 1] << 8));
  }
  return 1;
}


/**************************************************************************/
/*!
    @brief  Sends FeliCa Request Service command

    @param[out]  mode         Current Mode of the card
    @return                   = 1: Success
                              < 0: error
*/
/**************************************************************************/
int8_t PN532::felica_RequestResponse(uint8_t *mode) {
  uint8_t cmd[9];
  cmd[0] = FELICA_CMD_REQUEST_RESPONSE;
  memcpy(&cmd[1], _felicaIDm, 8);

  uint8_t response[10];
  uint8_t responseLength;
  if (felica_SendCommand(cmd, 9, response, &responseLength) != 1) {
    return -1;
  }

  // length check
  if (responseLength != 10) {
    return -2;
  }

  *mode = response[9];
  return 1;
}

/**************************************************************************/
/*!
    @brief  Sends FeliCa Read Without Encryption command

    @param[in]  numService         Length of the serviceCodeList
    @param[in]  serviceCodeList    Service Code List (Big Endian)
    @param[in]  numBlock           Length of the blockList
    @param[in]  blockList          Block List (Big Endian, This API only accepts 2-byte block list element)
    @param[out] blockData          Block Data
    @return                        = 1: Success
                                   < 0: error
*/
/**************************************************************************/
int8_t PN532::felica_ReadWithoutEncryption(uint8_t numService, const uint16_t *serviceCodeList, uint8_t numBlock, const uint16_t *blockList, uint8_t blockData[][16]) {
  if (numService > FELICA_READ_MAX_SERVICE_NUM) {
    return -1;
  }
  if (numBlock > FELICA_READ_MAX_BLOCK_NUM) {
    return -2;
  }

  uint8_t i, j = 0, k;
  uint8_t cmdLen = 1 + 8 + 1 + 2 * numService + 1 + 2 * numBlock;
  uint8_t cmd[cmdLen];
  cmd[j++] = FELICA_CMD_READ_WITHOUT_ENCRYPTION;
  for (i = 0; i < 8; ++i) {
    cmd[j++] = _felicaIDm[i];
  }
  cmd[j++] = numService;
  for (i = 0; i < numService; ++i) {
    cmd[j++] = serviceCodeList[i] & 0xFF;
    cmd[j++] = (serviceCodeList[i] >> 8) & 0xff;
  }
  cmd[j++] = numBlock;
  for (i = 0; i < numBlock; ++i) {
    cmd[j++] = (blockList[i] >> 8) & 0xFF;
    cmd[j++] = blockList[i] & 0xff;
  }

  uint8_t response[12 + 16 * numBlock];
  uint8_t responseLength;
  if (felica_SendCommand(cmd, cmdLen, response, &responseLength) != 1) {
    return -3;
  }

  // length check
  if (responseLength != 12 + 16 * numBlock) {
    return -4;
  }

  // status flag check
  if (response[9] != 0 || response[10] != 0) {
    return -5;
  }

  k = 12;
  for (i = 0; i < numBlock; i++) {
    for (j = 0; j < 16; j++) {
      blockData[i][j] = response[k++];
    }
  }

  return 1;
}


/**************************************************************************/
/*!
    @brief  Sends FeliCa Write Without Encryption command

    @param[in]  numService         Length of the serviceCodeList
    @param[in]  serviceCodeList    Service Code List (Big Endian)
    @param[in]  numBlock           Length of the blockList
    @param[in]  blockList          Block List (Big Endian, This API only accepts 2-byte block list element)
    @param[in]  blockData          Block Data (each Block has 16 bytes)
    @return                        = 1: Success
                                   < 0: error
*/
/**************************************************************************/
int8_t PN532::felica_WriteWithoutEncryption(uint8_t numService, const uint16_t *serviceCodeList, uint8_t numBlock, const uint16_t *blockList, uint8_t blockData[][16]) {
  if (numService > FELICA_WRITE_MAX_SERVICE_NUM) {
    return -1;
  }
  if (numBlock > FELICA_WRITE_MAX_BLOCK_NUM) {
    return -2;
  }

  uint8_t i, j = 0, k;
  uint8_t cmdLen = 1 + 8 + 1 + 2 * numService + 1 + 2 * numBlock + 16 * numBlock;
  uint8_t cmd[cmdLen];
  cmd[j++] = FELICA_CMD_WRITE_WITHOUT_ENCRYPTION;
  for (i = 0; i < 8; ++i) {
    cmd[j++] = _felicaIDm[i];
  }
  cmd[j++] = numService;
  for (i = 0; i < numService; ++i) {
    cmd[j++] = serviceCodeList[i] & 0xFF;
    cmd[j++] = (serviceCodeList[i] >> 8) & 0xff;
  }
  cmd[j++] = numBlock;
  for (i = 0; i < numBlock; ++i) {
    cmd[j++] = (blockList[i] >> 8) & 0xFF;
    cmd[j++] = blockList[i] & 0xff;
  }
  for (i = 0; i < numBlock; ++i) {
    for (k = 0; k < 16; k++) {
      cmd[j++] = blockData[i][k];
    }
  }

  uint8_t response[11];
  uint8_t responseLength;
  if (felica_SendCommand(cmd, cmdLen, response, &responseLength) != 1) {
    return -3;
  }

  // length check
  if (responseLength != 11) {
    return -4;
  }

  // status flag check
  if (response[9] != 0 || response[10] != 0) {
    return -5;
  }

  return 1;
}

/**************************************************************************/
/*!
    @brief  Sends FeliCa Request System Code command

    @param[out] numSystemCode        Length of the systemCodeList
    @param[out] systemCodeList       System Code list (Array length should longer than 16)
    @return                          = 1: Success
                                     < 0: error
*/
/**************************************************************************/
int8_t PN532::felica_RequestSystemCode(uint8_t *numSystemCode, uint16_t *systemCodeList) {
  uint8_t cmd[9];
  cmd[0] = FELICA_CMD_REQUEST_SYSTEM_CODE;
  memcpy(&cmd[1], _felicaIDm, 8);

  uint8_t response[10 + 2 * 16];
  uint8_t responseLength;
  if (felica_SendCommand(cmd, 9, response, &responseLength) != 1) {
    return -1;
  }
  *numSystemCode = response[9];

  // length check
  if (responseLength < 10 + 2 * *numSystemCode) {
    return -2;
  }

  uint8_t i;
  for (i = 0; i < *numSystemCode; i++) {
    systemCodeList[i] = (uint16_t)((response[10 + i * 2] << 8) + response[10 + i * 2 + 1]);
  }

  return 1;
}


/**************************************************************************/
/*!
    @brief  Release FeliCa card
    @return                          = 1: Success
                                     < 0: error
*/
/**************************************************************************/
int8_t PN532::felica_Release() {
  // InRelease
  pn532_packetbuffer[0] = PN532_COMMAND_INRELEASE;
  pn532_packetbuffer[1] = 0x00;  // All target

  if (HAL(writeCommand)(pn532_packetbuffer, 2)) {
    return -1;  // no ACK
  }

  // Wait card response
  int16_t frameLength = HAL(readResponse)(pn532_packetbuffer, sizeof(pn532_packetbuffer), 1000);
  if (frameLength < 0) {
    return -2;
  }

  // Check status (pn532_packetbuffer[0])
  if ((pn532_packetbuffer[0] & 0x3F) != 0) {
    return -3;
  }

  return 1;
}
