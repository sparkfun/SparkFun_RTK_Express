/*
  Test output of analog mux for different signals to DATA port

  0: ZED TX out/RX in
  1: PPS out/Ext Int in
  2: SCL out/SDA in
  3: ESP26 DAC out/ESP39 ADC in
*/

const int muxA = 2;
const int muxB = 4;

#include <Wire.h>

void setup()
{
  Serial.begin(115200);
  Serial.println("Mux testing");

  pinMode(muxA, OUTPUT);
  pinMode(muxB, OUTPUT);

  Serial.println("0) ZED TX out/RX in");
  Serial.println("1) PPS out/Ext Int in");
  Serial.println("2) SCL out/SDA in");
  Serial.println("3) ESP26 DAC out/ESP39 ADC in");
}

void loop()
{
  if (Serial.available())
  {
    byte incoming = Serial.read();

    if (incoming == '0')
    {
      setMux(0);
      Serial.println("ZED TX out/RX in");
    }
    else if (incoming == '1')
    {
      setMux(1);
      Serial.println("PPS out/Ext Int in");
    }
    else if (incoming == '2')
    {
      setMux(2);
      Serial.println("SCL out/SDA in");

      while (Serial.available()) Serial.read();
      Wire.begin();
      scanner();

    }
    else if (incoming == '3')
    {
      setMux(3);
      Serial.println("ESP26 DAC out/ESP39 ADC in");
    }
  }

}

void scanner()
{
  while (1)
  {
    if (Serial.available()) return;

    Serial.println();
    for (byte address = 1; address < 127; address++ )
    {
      Wire.beginTransmission(address);
      if (Wire.endTransmission() == 0)
      {
        Serial.print("Device found at address 0x");
        if (address < 0x10)
          Serial.print("0");
        Serial.println(address, HEX);
      }
    }
    Serial.println("Done");
    delay(100);
  }
}

void setMux(int channelNumber)
{
  if (channelNumber > 3) return; //Error check

  switch (channelNumber)
  {
    case 0:
      digitalWrite(muxA, LOW);
      digitalWrite(muxB, LOW);
      break;
    case 1:
      digitalWrite(muxA, HIGH);
      digitalWrite(muxB, LOW);
      break;
    case 2:
      digitalWrite(muxA, LOW);
      digitalWrite(muxB, HIGH);
      break;
    case 3:
      digitalWrite(muxA, HIGH);
      digitalWrite(muxB, HIGH);
      break;
  }
}
