void beginDisplay()
{
  //0x3D is default on Qwiic board
  if (isConnected(0x3D) == true || isConnected(0x3C) == true)
  {
    //online.display = true;

    //Init and display splash
    oled.begin();     // Initialize the OLED
    oled.clear(PAGE); // Clear the display's internal memory

    oled.setCursor(15, 3); //x, y
    oled.setFontType(0); //Set font to smallest
    oled.print(F("Window"));

    int textX = 12;
    int textY = 25;
    int textKerning = 9;
    oled.setFontType(1);
    printTextwithKerning("Check", textX, textY, textKerning);

  int xMax = 63;
  int yMax = 47;

  oled.line(0, 0, xMax, 0); //Top
  oled.line(0, 0, 0, yMax); //Left
  oled.line(0, yMax, xMax, yMax); //Bottom
  oled.line(xMax, 0, xMax, yMax); //Right
    
    oled.display();
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

void displayError(char * errorMessage)
{
  oled.clear(PAGE); // Clear the display's internal buffer

  oled.setCursor(2, 2); //x, y
  oled.setFontType(0); //Set font to smallest
  oled.print(F("Error:"));

  oled.setCursor(2, 10);
  //oled.setFontType(1);
  oled.print(errorMessage);

  oled.display(); //Push internal buffer to display
}
