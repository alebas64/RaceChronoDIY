#include "BoardSetup.h"

void setup()
{
  Serial.begin(115200);

  setupBoards();

}

void loop()
{

  if (Serial.available())
  {
    SerialGPS.write(Serial.read()); // read it and send it out Serial1 (pins 0 & 1)
  }

  if (SerialGPS.available())
  {
    Serial.write(SerialGPS.read()); // read it and send it out Serial (USB)
  }
}
