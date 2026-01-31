#ifndef UBLOX_FUNCTIONS_H
#define UBLOX_FUNCTIONS_H

#include "BoardSetup.h"
#include "ublox_defines.h"

#define UBX_ID_NAV_DOP 0x04
#define UBX_ID_NAV_PVT 0x07

// Checksum calculation for UBLOX module
void _calcChecksum(uint8_t *CK, uint8_t *payload, uint16_t length);

// U-blox receiver disable NMEA messages
void ublox_noNMEA();

// Function to send bytearray to ublox receiver over serial
void ublox_sendPacket(uint8_t *packet, uint8_t len);

// U-blox receiver disable NMEA messages
void ublox_noNMEA();

// U-blox receiver change baudrate to 115200
void ublox_setBaudrate();

// U-blox receiver change frequency to 10Hz
void ublox_changeFrequency(uint8_t hz);

// U-blox receiver enable NAV-PVT messages
void ublox_enableNavPvt();

// U-blox receiver enable NAV-DOP messages
void ublox_enableNavDop();


#endif