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
  oled.print(F("SparkFun"));

  oled.setCursor(21, 13);
  oled.setFontType(1);
  oled.print(F("RTK"));

  int textX = 3;
  int textY = 25;
  int textKerning = 9;
  oled.setFontType(1);
  printTextwithKerning("Express", textX, textY, textKerning);

  oled.setCursor(20, 41);
  oled.setFontType(0); //Set font to smallest
  oled.printf("v%d.%d", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);
  oled.display();
}

void displayShutdown()
{
  //Show shutdown text
  oled.clear(PAGE); // Clear the display's internal memory

  oled.setCursor(21, 13);
  oled.setFontType(1);

  int textX = 2;
  int textY = 10;
  int textKerning = 8;

  printTextwithKerning("Shutting", textX, textY, textKerning);

  textX = 4;
  textY = 25;
  textKerning = 9;
  oled.setFontType(1);

  printTextwithKerning("Down...", textX, textY, textKerning);

  oled.display();
}
