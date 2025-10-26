#include "ublox_functions.h"

// Checksum calculation for UBLOX module
void _calcChecksum(uint8_t *CK, uint8_t *payload, uint16_t length)
{
  CK[0] = 0;
  CK[1] = 0;

  for (uint8_t i = 0; i < length; i++)
  {
    CK[0] += payload[i];
    CK[1] += CK[0];
  }
}

// Function to send bytearray to ublox receiver over serial
void ublox_sendPacket(uint8_t *packet, uint8_t len)
{
  for (size_t i = 0; i < len; i++)
  {
    SerialGPS.write(packet[i]);
  }
}

// U-blox receiver disable NMEA messages
void ublox_noNMEA()
{
  uint8_t messages[][2] = {
      {0xF0, 0x0A},
      {0xF0, 0x09},
      {0xF0, 0x00},
      {0xF0, 0x01},
      {0xF0, 0x0D},
      {0xF0, 0x06},
      {0xF0, 0x02},
      {0xF0, 0x07},
      {0xF0, 0x03},
      {0xF0, 0x04},
      {0xF0, 0x0E},
      {0xF0, 0x0F},
      {0xF0, 0x05},
      {0xF0, 0x08},
      {0xF1, 0x00},
      {0xF1, 0x01},
      {0xF1, 0x03},
      {0xF1, 0x04},
      {0xF1, 0x05},
      {0xF1, 0x06},
  };

  uint8_t packet[] = {
      0xB5, // sync 1
      0x62, // sync  2
      0x06, // class
      0x01, // id
      0x08, // length
      0x00, // length
      0x00, // payload (first byte from messages array element)
      0x00, // payload (second byte from messages array element)
      0x00, // payload (not changed in the case)
      0x00, // payload (not changed in the case)
      0x00, // payload (not changed in the case)
      0x00, // payload (not changed in the case)
      0x00, // payload (not changed in the case)
      0x00, // payload (not changed in the case)
      0x00, // CK_A
      0x00, // CK_B
  };

  uint8_t packetSize = sizeof(packet);

  // Offset to the place where payload starts.
  uint8_t payloadOffset = 6;

  // Iterate over the messages array.
  for (size_t i = 0; i < sizeof(messages) / sizeof(*messages); i++)
  {
    // Copy two bytes of payload to the packet buffer.
    for (size_t j = 0; j < sizeof(*messages); j++)
    {
      packet[payloadOffset + j] = messages[i][j];
    }

    // Set checksum bytes to the null.
    packet[packetSize - 2] = 0x00;
    packet[packetSize - 1] = 0x00;
    _calcChecksum(&packet[packetSize - 2], &packet[2], (packetSize - 4));

    ublox_sendPacket(packet, packetSize);
  }
}


// U-blox receiver change baudrate to 115200
void ublox_setBaudrate()
{
  uint8_t packet[] = {
      0xB5, // sync 1
      0x62, // sync 2
      0x06, // class
      0x00, // id
      0x14, // length
      0x00, // length
      0x01, // payload
      0x00, // payload
      0x00, // payload
      0x00, // payload
      0xD0, // payload
      0x08, // payload
      0x00, // payload
      0x00, // payload
      0x00, // payload
      0xC2, // payload
      0x01, // payload
      0x00, // payload
      0x07, // payload
      0x00, // payload
      0x03, // payload
      0x00, // payload
      0x00, // payload
      0x00, // payload
      0x00, // payload
      0x00, // payload
      0xC0, // CK_A
      0x7E, // CK_B
  };
  ublox_sendPacket(packet, sizeof(packet));
}

// U-blox receiver change frequency to 10Hz
void ublox_changeFrequency()
{
  uint8_t packet[] = {
      0xB5, // sync 1
      0x62, // sync 2
      0x06, // class
      0x08, // id
      0x06, // length
      0x00, // length
      0x64, // payload
      0x00, // payload
      0x01, // payload
      0x00, // payload
      0x01, // payload
      0x00, // payload
      0x7A, // CK_A
      0x12, // CK_B
  };
  ublox_sendPacket(packet, sizeof(packet));
}

// U-blox receiver enable NAV-PVT messages
void ublox_enableNavPvt()
{
  // CFG-MSG packet.
  uint8_t packet[] = {
      0xB5, // sync 1
      0x62, // sync 2
      0x06, // class
      0x01, // id
      0x08, // length
      0x00, // length
      0x01, // payload
      0x07, // payload
      0x01, // payload
      0x01, // payload
      0x00, // payload
      0x01, // payload
      0x01, // payload
      0x00, // payload
      0x1B, // CK_A
      0xEC, // CK_B
  };
  ublox_sendPacket(packet, sizeof(packet));
}

// U-blox receiver enable NAV-DOP messages
void ublox_enableNavDop()
{
  // CFG-MSG packet.
  uint8_t packet[] = {
      0xB5, // sync 1
      0x62, // sync 2
      0x06, // class
      0x01, // id
      0x08, // length
      0x00, // length
      0x01, // payload
      0x04, // payload
      0x01, // payload
      0x01, // payload
      0x00, // payload
      0x01, // payload
      0x01, // payload
      0x00, // payload
      0x18, // CK_A
      0xD7, // CK_B
  };
  ublox_sendPacket(packet, sizeof(packet));
}

