#include "BoardSetup.h"
#include "ublox_defines.h"

#define RACECHRONO_UUID "00001ff8-0000-1000-8000-00805f9b34fb" // RaceChrono service UUID
#define UBX_ID_NAV_DOP 0x04
#define UBX_ID_NAV_PVT 0x07

bool deviceConnected = false;
bool oldDeviceConnected = false;

int gpsPreviousDateAndHour = 0;
int dateAndHour;
int timeSinceHourStart;

uint8_t _byte;
uint8_t _parserState;
uint8_t _tempPacket[255];
uint8_t rc_data[20];
uint8_t gpsSyncBits = 0;

int16_t msg_length; 

BLEServer *BLE_server = NULL;
BLECharacteristic *BLE_GPS_Main_Characteristic = NULL; // RaceChrono GPS Main characteristic UUID 0x03
BLECharacteristic *BLE_GPS_Time_Characteristic = NULL; // RaceChrono GPS Time characteristic UUID 0x04

String device_name = "RC_DIY_" + String((uint16_t)((uint64_t)ESP.getEfuseMac() >> 32));

// Bluetooth Low Energy BLEServerCallbacks
class ServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *BLE_server)
  {
    deviceConnected = true;
    Serial.println("[I] Bluetooth client connected!");
  };

  void onDisconnect(BLEServer *BLE_server)
  {
    deviceConnected = false;
    Serial.println("[I] Bluetooth client disconnected!");
  }
};

// BLE configuration
void configBLE()
{
  BLEDevice::init(device_name.c_str());
  BLE_server = BLEDevice::createServer();
  BLE_server->setCallbacks(new ServerCallbacks());
  BLEService *BLE_service = BLE_server->createService(RACECHRONO_UUID);

  // GPS main characteristic definition
  BLE_GPS_Main_Characteristic = BLE_service->createCharacteristic(BLEUUID((uint16_t)0x3), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
  BLE_GPS_Main_Characteristic->addDescriptor(new BLE2902());

  // GPS time characteristic definition
  BLE_GPS_Time_Characteristic = BLE_service->createCharacteristic(BLEUUID((uint16_t)0x4), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
  BLE_GPS_Time_Characteristic->addDescriptor(new BLE2902());

  BLE_service->start();

  BLEAdvertising *BLE_advertising = BLEDevice::getAdvertising();
  BLE_advertising->addServiceUUID(RACECHRONO_UUID);
  BLE_advertising->setScanResponse(false);
  BLE_advertising->setMinInterval(100);
  BLE_advertising->setMaxInterval(100);
  BLEDevice::startAdvertising();
}

// Setup UART/BLE/GPS/AXP
void setup()
{
  // ESP32 UART - 115200
  Serial.begin(115200);
  setupBoards();
  configBLE();

  // LED blink fast when ready
  //axp.setChgLEDMode(AXP20X_LED_BLINK_4HZ);
}

// U-blox read incoming messages
bool read_ublox()
{
  uint8_t _checksum[2];
  const uint8_t _ubxHeader[2] = {0xB5, 0x62};
  while (SerialGPS.available())
  {
    _byte = SerialGPS.read();
    if (_parserState < 2)
    {
      if (_byte == _ubxHeader[_parserState])
      {
        _parserState++;
      }
      else
      {
        _parserState = 0;
      }
    }
    else
    {
      if (_parserState == 2)
      {
        if (_byte == 1)
        { // NAV
          msg_length = 2;
        }
      }
      if (_parserState == 3)
      {
        if (_byte == UBX_ID_NAV_DOP)
        {
          msg_length = 22; // 18+4
        }
        else if (_byte == UBX_ID_NAV_PVT)
        {
          msg_length = 96; // 92+4
        }
        else
        {
          msg_length = 0;
        }
      }
      if ((_parserState - 2) < msg_length)
      {
        *((uint8_t *)&_tempPacket + _parserState - 2) = _byte;
      }
      _parserState++;
      // compute checksum
      if ((_parserState - 2) == msg_length)
      {
        _calcChecksum(_checksum, ((uint8_t *)&_tempPacket), msg_length);
      }
      else if ((_parserState - 2) == (msg_length + 1))
      {
        if (_byte != _checksum[0])
        {
          _parserState = 0;
        }
      }
      else if ((_parserState - 2) == (msg_length + 2))
      {
        _parserState = 0;
        if (_byte == _checksum[1])
        {
          memcpy(&_validPacket, &_tempPacket, sizeof(_validPacket));
          return true;
        }
      }
      else if (_parserState > (msg_length + 4))
      {
        _parserState = 0;
      }
    }
  }
  return false;
}

uint32_t oled_millis_update = 0;
const uint32_t oled_update_interval = 1000;

// Main loop
void loop()
{
  if(millis() - oled_millis_update > oled_update_interval) {
    oled_millis_update = millis();
    // Update display or other periodic tasks
    Serial.printf("[I] battery: %.2fV\n", 0.0);
    Serial.printf("[i] gps speed: %.2f km/h\n", (_validPacket.pvt.gSpeed * 0.0036));
  }

  if (deviceConnected)
  {
    if (read_ublox())
    {
      //Serial.println("[I] Valid UBLOX packet received!");
      if (_validPacket.pvt.message_id == UBX_ID_NAV_PVT)
      {
        //Serial.println("[I] Processing NAV-PVT packet...");
        dateAndHour = (_validPacket.pvt.year - 2000) * 8928 + (_validPacket.pvt.month - 1) * 744 + (_validPacket.pvt.day - 1) * 24 + _validPacket.pvt.hour;

        /*
         * UBLOX   -> scaling:N/A unit:min name:min  desc:Minute of hour, range 0..59 (UTC)
         *   -> scaling:N/A unit:s   name:sec  desc:Seconds of minute, range 0..60 (UTC)
         *   -> scaling:N/A unit:ns  name:nano desc:Fraction of second, range -1e9 .. 1e9 (UTC)
         * RaceChrono -> time from hour start = (minute * 30000) + (seconds * 500) + (milliseconds / 2)
         */
        timeSinceHourStart = _validPacket.pvt.min * 30000 + _validPacket.pvt.sec * 500 + (_validPacket.pvt.nano / 1000000.0) / 2;

        /*
         * UBLOX   -> scaling:1e-5 unit:deg name:headMot desc:Heading of motion (2-D)
         * RaceChrono -> scaling:1e+2 unit:deg
         */
        int gps_bearing = max(0.0, round((double)_validPacket.pvt.headMot / 1000.0));

        /*
         * UBLOX   -> scaling:N/A unit:mm name:hMSL desc:Height above mean sea level
         * RaceChrono -> scaling:N/A unit:deg
         */
        int gps_altitude = ((double)_validPacket.pvt.hMSL * 1e-3) > 6000.f ? ((int)max(0.0, round(((double)_validPacket.pvt.hMSL * 1e-3) + 500.f)) & 0x7FFF) | 0x8000 : (int)max(0.0, round((((double)_validPacket.pvt.hMSL * 1e-3) + 500.f) * 10.f)) & 0x7FFF;

        /*
         * UBLOX   -> scaling:N/A unit:mm/s name:gSpeed desc:Ground Speed (2-D)
         * RaceChrono -> scaling:N/A unit:km/h
         */
        int gps_speed = ((double)_validPacket.pvt.gSpeed * 0.0036) > 600.f ? ((int)(max(0.0, round(((double)_validPacket.pvt.gSpeed * 0.0036) * 10.f))) & 0x7FFF) | 0x8000 : (int)(max(0.0, round(((double)_validPacket.pvt.gSpeed * 0.0036) * 100.f))) & 0x7FFF;

        rc_data[0] = ((gpsSyncBits & 0x7) << 5) | ((timeSinceHourStart >> 16) & 0x1F);
        rc_data[1] = timeSinceHourStart >> 8;
        rc_data[2] = timeSinceHourStart;
        rc_data[3] = ((min(0x03, (int)_validPacket.pvt.fixType) & 0x3) << 6) | ((min(0x3F, (int)_validPacket.pvt.numSV)) & 0x3F);
        rc_data[4] = _validPacket.pvt.lat >> 24;
        rc_data[5] = _validPacket.pvt.lat >> 16;
        rc_data[6] = _validPacket.pvt.lat >> 8;
        rc_data[7] = _validPacket.pvt.lat;
        rc_data[8] = _validPacket.pvt.lon >> 24;
        rc_data[9] = _validPacket.pvt.lon >> 16;
        rc_data[10] = _validPacket.pvt.lon >> 8;
        rc_data[11] = _validPacket.pvt.lon;
        rc_data[12] = gps_altitude >> 8;
        rc_data[13] = gps_altitude;
        rc_data[14] = gps_speed >> 8;
        rc_data[15] = gps_speed;
        rc_data[16] = gps_bearing >> 8;
        rc_data[17] = gps_bearing;
      }
      else if (_validPacket.dop.message_id == UBX_ID_NAV_DOP)
      {
        /*
         * UBLOX   -> scaling:1e-2 unit:N/A name:hDOP des:Horizontal DOP
         *   -> scaling:1e-2 unit:N/A name:vDOP des:Vertical DOP
         * RaceChrono -> scaling:1e1  unit:N/A
         */
        rc_data[18] = _validPacket.dop.hDOP / 10;
        rc_data[19] = _validPacket.dop.vDOP / 10;
      }

      BLE_GPS_Main_Characteristic->setValue(rc_data, 20);
      BLE_GPS_Main_Characteristic->notify();

      if (gpsPreviousDateAndHour != dateAndHour)
      {
        gpsPreviousDateAndHour = dateAndHour;
        gpsSyncBits++;
        rc_data[0] = ((gpsSyncBits & 0x7) << 5) | ((dateAndHour >> 16) & 0x1F);
        rc_data[1] = dateAndHour >> 8;
        rc_data[2] = dateAndHour;
        BLE_GPS_Time_Characteristic->setValue(rc_data, 3);
        BLE_GPS_Time_Characteristic->notify();
      }
    }
  }
  if (!deviceConnected && oldDeviceConnected)
  {
    delay(500);
    BLE_server->startAdvertising();
    //axp.setChgLEDMode(AXP20X_LED_BLINK_4HZ);
    Serial.println("[I] Bluetooth device discoverable");
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected)
  {
    //axp.setChgLEDMode(AXP20X_LED_BLINK_1HZ);
    oldDeviceConnected = deviceConnected;
  }
}
