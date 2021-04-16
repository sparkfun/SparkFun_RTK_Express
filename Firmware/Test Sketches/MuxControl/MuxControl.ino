/*
  Test output of analog mux for different signals to DATA port

  0: ZED TX out/RX in
  1: PPS out/Ext Int in
  2: SCL out/SDA in
  3: ESP26 DAC out/ESP39 ADC in
*/

//const int muxA = 2;
//const int muxB = 4;

//Facet v10
const int muxA = 4;
const int muxB = 5;

#include <Wire.h>

void setup()
{
  Serial.begin(115200);
  delay(250);
  Serial.println("Mux testing");

      Wire.begin();

  pinMode(muxA, OUTPUT);
  pinMode(muxB, OUTPUT);
  setMuxport(0);

  pinMode(17, INPUT); //Put serial TXpin into high impedance to avoid contention with outside TX. May not be needed. 1K inline.

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
      setMuxport(0);
      Serial.println("ZED TX out/RX in");
    }
    else if (incoming == '1')
    {
      setMuxport(1);
      Serial.println("PPS out/Ext Int in");
    }
    else if (incoming == '2')
    {
      setMuxport(2);
      Serial.println("SCL out/SDA in");

      delay(10);
      while (Serial.available()) Serial.read();
      Serial.println("Scanning for I2C devices");
      scanner();

    }
    else if (incoming == '3')
    {
      setMuxport(3);
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
      Serial.flush();
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

  Serial.println("Returning from scanner");
}

void setMuxport(int channelNumber)
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
