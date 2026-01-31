#ifndef UBLOX_DEFINES_H
#define UBLOX_DEFINES_H

#include <stdint.h>

struct ublox
{
  uint8_t message_class;
  uint8_t message_id;
  uint16_t payload_length;
};

//144 bit
struct ublox_NAV_DOP : ublox
{
  uint32_t iTOW;
  uint16_t gDOP;
  uint16_t pDOP;
  uint16_t tDOP;
  uint16_t vDOP;
  uint16_t hDOP;
  uint16_t nDOP;
  uint16_t eDOP;
};

//736 bit
struct ublox_NAV_PVT : ublox
{
  uint32_t iTOW;
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
  uint8_t valid;
  uint32_t tAcc;
  int32_t nano;
  uint8_t fixType;
  uint8_t flags;
  uint8_t flags2;
  uint8_t numSV;
  int32_t lon;
  int32_t lat;
  int32_t height;
  int32_t hMSL;
  uint32_t hAcc;
  uint32_t vAcc;
  int32_t velN;
  int32_t velE;
  int32_t velD;
  int32_t gSpeed;
  int32_t headMot;
  uint32_t sAcc;
  uint32_t headAcc;
  uint16_t pDOP;
  uint16_t reserved2;
  uint32_t reserved3;
  int32_t headVeh;
  int16_t magDec;
  uint16_t magAcc;
};

//prob 880 bit = 110 byte
union
{
  ublox_NAV_DOP dop;
  ublox_NAV_PVT pvt;
} _validPacket;

/*
typedef struct{
  ublox_NAV_DOP dop;
  ublox_NAV_PVT pvt;
}valid_packet_t;
*/
#endif // UBLOX_DEFINES_H