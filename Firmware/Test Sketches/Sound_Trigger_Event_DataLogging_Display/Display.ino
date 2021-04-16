//Ping an I2C device and see if it responds
bool isConnected(uint8_t deviceAddress)
{
  Wire.beginTransmission(deviceAddress);
  if (Wire.endTransmission() == 0)
    return true;
  return false;
}

void beginDisplay()
{
  //0x3D is default on Qwiic board
  if (isConnected(0x3D) == true || isConnected(0x3C) == true)
  {
    displaySplash();
  }
  else
  {
    Serial.println("Error: Display didn't ack!");
  }
}

//Given text, a position, and kerning, print text to display
//This is helpful for squishing or stretching a string to appropriately fill the display
void printTextwithKerning(char *newText, uint8_t xPos, uint8_t yPos, uint8_t kerning)
{
  for (int x = 0 ; x < strlen(newText) ; x++)
  {
    oled.setCursor(xPos, yPos);
    oled.print(newText[x]);
    xPos += kerning;
  }
}

void displaySplash()
{
  //Init and display splash
  oled.begin();     // Initialize the OLED
  oled.clear(PAGE); // Clear the display's internal memory

  oled.setCursor(10, 2); //x, y
  oled.setFontType(0); //Set font to smallest
  oled.print(F("Mic"));

  oled.setCursor(1, 13);
  oled.setFontType(1);
  oled.print(F("Mapper"));

  int textX = 3;
  int textY = 25;
  int textKerning = 9;
  oled.setFontType(1);
  printTextwithKerning("Express", textX, textY, textKerning);

  oled.display();
}
