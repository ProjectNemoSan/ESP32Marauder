#include "Display.h"
#include "lang_var.h"

#ifdef HAS_SCREEN

Display::Display()
#ifdef HAS_CYD_TOUCH
  : touchscreenSPI(VSPI),
    touchscreen(XPT2046_CS, XPT2046_IRQ)
#endif
{
}

int8_t Display::menuButton(uint16_t *x, uint16_t *y, bool pressed, bool check_hold) {
  #ifdef HAS_ILI9341
    for (uint8_t b = BUTTON_ARRAY_LEN; b < BUTTON_ARRAY_LEN + 3; b++) {
      if (pressed && this->key[b].contains(*x, *y)) {
        this->key[b].press(true);  // tell the button it is pressed
      } else {
        this->key[b].press(false);  // tell the button it is NOT pressed
      }
    }

    for (uint8_t b = BUTTON_ARRAY_LEN; b < BUTTON_ARRAY_LEN + 3; b++) {
      if (!check_hold) {
        if ((this->key[b].justReleased()) && (!pressed)) {
          return b - BUTTON_ARRAY_LEN;
        }
      }
      else {
        if ((this->key[b].isPressed())) {
          return b - BUTTON_ARRAY_LEN;
        }
      }
    }

  #endif

  return -1;
}

uint8_t Display::updateTouch(uint16_t *x, uint16_t *y, uint16_t threshold) {
  #ifdef HAS_ILI9341
    if (!this->headless_mode)
      #ifndef HAS_CYD_TOUCH
        return this->tft.getTouch(x, y, threshold);
      #else
        if (this->touchscreen.tirqTouched() && this->touchscreen.touched()) {
          TS_Point p = this->touchscreen.getPoint();

          //*x = map(p.x, 200, 3700, 1, TFT_WIDTH);
          //*y = map(p.y, 240, 3800, 1, TFT_HEIGHT);

          uint8_t rot = this->tft.getRotation();

          //#ifdef HAS_CYD_PORTRAIT
          //  rot = 0;
          //#endif

          switch (rot) {
            case 0: // Standard Protrait
              *x = map(p.x, 200, 3700, 1, TFT_WIDTH);
              *y = map(p.y, 240, 3800, 1, TFT_HEIGHT);
              break;
            case 1:
              *x = map(p.y, 143, 3715, 0, TFT_HEIGHT);     // Horizontal (Y axis in touch, X on screen)
              *y = map(p.x, 3786, 216, 0, TFT_WIDTH);    // Vertical (X axis in touch, Y on screen)
              break;
            case 2:
              *x = map(p.x, 3700, 200, 1, TFT_WIDTH);
              *y = map(p.y, 3800, 240, 1, TFT_HEIGHT);
              break;
            case 3:
              *x = map(p.y, 3800, 240, 1, TFT_WIDTH);
              *y = map(p.x, 200, 3700, 1, TFT_HEIGHT);
              break;
          }
          return 1;
        }
        else
          return 0;
      #endif
    else
      return !this->headless_mode;
  #endif

  return 0;
}

bool Display::isTouchHeld(uint16_t threshold) {
  static unsigned long touchStartTime = 0;
  static bool touchHeld = false;
  uint16_t x, y;

  if (this->updateTouch(&x, &y, threshold)) {
    // Touch detected
    if (touchStartTime == 0) {
      touchStartTime = millis();  // First touch timestamp
    } else if (!touchHeld && millis() - touchStartTime >= 1000) {
      touchHeld = true;  // Held for at least 1000ms
      return true;
    }
  } else {
    // Touch released
    touchStartTime = 0;
    touchHeld = false;
  }

  return false;
}

// Function to prepare the display and the menus
void Display::RunSetup()
{
  run_setup = false;

  // Need to declare new
  display_buffer = new LinkedList<String>();

  #ifdef SCREEN_BUFFER
    screen_buffer = new LinkedList<String>();
  #endif

  #ifdef HAS_CYD_TOUCH
    this->touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    this->touchscreen.begin(touchscreenSPI);
    this->touchscreen.setRotation(0);
  #endif
  
  tft.init();

  tft.setRotation(SCREEN_ORIENTATION);

  tft.setCursor(0, 0);

  #ifdef HAS_ILI9341

    #ifndef HAS_CYD_TOUCH
      #ifdef TFT_SHIELD
        uint16_t calData[5] = { 275, 3494, 361, 3528, 4 }; // tft.setRotation(0); // Portrait with TFT Shield
      #elif defined(TFT_DIY)
        uint16_t calData[5] = { 339, 3470, 237, 3438, 2 }; // tft.setRotation(0); // Portrait with DIY TFT
      #endif
      tft.setTouch(calData);
    #endif

  #endif

  clearScreen();

  #ifdef KIT
    pinMode(KIT_LED_BUILTIN, OUTPUT);
  #endif

  #ifdef MARAUDER_REV_FEATHER
    pinMode(7, OUTPUT);

    delay(10);

    digitalWrite(7, HIGH);
  #endif
}

void Display::drawFrame()
{
  tft.drawRect(FRAME_X, FRAME_Y, FRAME_W, FRAME_H, TFT_BLACK);
}

void Display::tftDrawRedOnOffButton() {
  tft.fillRect(REDBUTTON_X, REDBUTTON_Y, REDBUTTON_W, REDBUTTON_H, TFT_RED);
  tft.fillRect(GREENBUTTON_X, GREENBUTTON_Y, GREENBUTTON_W, GREENBUTTON_H, TFT_DARKGREY);
  drawFrame();
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(text03, GREENBUTTON_X + (GREENBUTTON_W / 2), GREENBUTTON_Y + (GREENBUTTON_H / 2));
  this->SwitchOn = false;
}

void Display::tftDrawGreenOnOffButton() {
  tft.fillRect(GREENBUTTON_X, GREENBUTTON_Y, GREENBUTTON_W, GREENBUTTON_H, TFT_GREEN);
  tft.fillRect(REDBUTTON_X, REDBUTTON_Y, REDBUTTON_W, REDBUTTON_H, TFT_DARKGREY);
  drawFrame();
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(text04, REDBUTTON_X + (REDBUTTON_W / 2) + 1, REDBUTTON_Y + (REDBUTTON_H / 2));
  this->SwitchOn = true;
}

void Display::tftDrawGraphObjects(byte x_scale)
{
  //draw the graph objects
  tft.fillRect(11, 5, x_scale+1, 120, TFT_BLACK); // positive start point
  tft.fillRect(11, 121, x_scale+1, 119, TFT_BLACK); // negative start point
  tft.drawFastVLine(10, 5, 230, TFT_WHITE); // y axis
  tft.drawFastHLine(10, HEIGHT_1 - 1, 310, TFT_WHITE); // x axis
  tft.setTextColor(TFT_YELLOW); tft.setTextSize(1); // set parameters for y axis labels
  //tft.setCursor(3, 116); tft.print(midway);  // "0" at center of ya axis
  tft.setCursor(3, 6); tft.print("+"); // "+' at top of y axis
  tft.setCursor(3, 228); tft.print("0"); // "-" at bottom of y axis
}

void Display::tftDrawEapolColorKey()
{
  //Display color key
  tft.setTextSize(1); tft.setTextColor(TFT_WHITE);
  tft.fillRect(14, 0, 15, 8, TFT_CYAN); tft.setCursor(30, 0); tft.print(" - EAPOL"); 
}

void Display::tftDrawColorKey()
{
  //Display color key
  tft.setTextSize(1); tft.setTextColor(TFT_WHITE);
  tft.fillRect(14, 0, 15, 8, TFT_GREEN); tft.setCursor(30, 0); tft.print(" - Beacons"); 
  tft.fillRect(14, 8, 15, 8, TFT_RED); tft.setCursor(30, 8); tft.print(" - Deauths");
  tft.fillRect(14, 16, 15, 8, TFT_BLUE); tft.setCursor(30, 16); tft.print(" - Probes");
}

void Display::tftDrawXScaleButtons(byte x_scale)
{
  tft.drawFastVLine(234, 0, 20, TFT_WHITE);
  tft.setCursor(208, 21); tft.setTextColor(TFT_WHITE); tft.setTextSize(1); tft.print("X Scale:"); tft.print(x_scale);

  key[0].initButton(&tft, // x - box
                        220,
                        10, // x, y, w, h, outline, fill, text
                        20,
                        20,
                        TFT_BLACK, // Outline
                        TFT_CYAN, // Fill
                        TFT_BLACK, // Text
                        "-",
                        2);
  key[1].initButton(&tft, // x + box
                        249,
                        10, // x, y, w, h, outline, fill, text
                        20,
                        20,
                        TFT_BLACK, // Outline
                        TFT_CYAN, // Fill
                        TFT_BLACK, // Text
                        "+",
                        2);

  key[0].setLabelDatum(1, 5, MC_DATUM);
  key[1].setLabelDatum(1, 5, MC_DATUM);

  key[0].drawButton();
  key[1].drawButton();
}

void Display::tftDrawYScaleButtons(byte y_scale)
{
  tft.drawFastVLine(290, 0, 20, TFT_WHITE);
  tft.setCursor(265, 21); tft.setTextColor(TFT_WHITE); tft.setTextSize(1); tft.print("Y Scale:"); tft.print(y_scale);

  key[2].initButton(&tft, // y - box
                        276,
                        10, // x, y, w, h, outline, fill, text
                        20,
                        20,
                        TFT_BLACK, // Outline
                        TFT_MAGENTA, // Fill
                        TFT_BLACK, // Text
                        "-",
                        2);
  key[3].initButton(&tft, // y + box
                        305,
                        10, // x, y, w, h, outline, fill, text
                        20,
                        20,
                        TFT_BLACK, // Outline
                        TFT_MAGENTA, // Fill
                        TFT_BLACK, // Text
                        "+",
                        2);

  key[2].setLabelDatum(1, 5, MC_DATUM);
  key[3].setLabelDatum(1, 5, MC_DATUM);

  key[2].drawButton();
  key[3].drawButton();
}

void Display::tftDrawChannelScaleButtons(int set_channel, bool lnd_an)
{
  if (lnd_an) {
    tft.drawFastVLine(178, 0, 20, TFT_WHITE);
    tft.setCursor(145, 21); tft.setTextColor(TFT_WHITE); tft.setTextSize(1); tft.print(text10); tft.print(set_channel);

    key[4].initButton(&tft, // channel - box
                          164,
                          10, // x, y, w, h, outline, fill, text
                          EXT_BUTTON_WIDTH,
                          EXT_BUTTON_WIDTH,
                          TFT_BLACK, // Outline
                          TFT_BLUE, // Fill
                          TFT_BLACK, // Text
                          "-",
                          2);
    key[5].initButton(&tft, // channel + box
                          193,
                          10, // x, y, w, h, outline, fill, text
                          EXT_BUTTON_WIDTH,
                          EXT_BUTTON_WIDTH,
                          TFT_BLACK, // Outline
                          TFT_BLUE, // Fill
                          TFT_BLACK, // Text
                          "+",
                          2);
  }

  else {
    key[4].initButton(&tft, // channel - box
                          (EXT_BUTTON_WIDTH / 2) * 6,
                          (STATUS_BAR_WIDTH * 2) + CHAR_WIDTH - 1, // x, y, w, h, outline, fill, text
                          EXT_BUTTON_WIDTH,
                          EXT_BUTTON_WIDTH,
                          TFT_BLACK, // Outline
                          TFT_BLUE, // Fill
                          TFT_BLACK, // Text
                          "-",
                          2);
    key[5].initButton(&tft, // channel + box
                          (EXT_BUTTON_WIDTH / 2) * 10,
                          (STATUS_BAR_WIDTH * 2) + CHAR_WIDTH - 1, // x, y, w, h, outline, fill, text
                          EXT_BUTTON_WIDTH,
                          EXT_BUTTON_WIDTH,
                          TFT_BLACK, // Outline
                          TFT_BLUE, // Fill
                          TFT_BLACK, // Text
                          "+",
                          2);
  }

  key[4].setLabelDatum(1, 5, MC_DATUM);
  key[5].setLabelDatum(1, 5, MC_DATUM);

  key[4].drawButton();
  key[5].drawButton();
}

void Display::tftDrawExitScaleButtons(bool lnd_an)
{
  //tft.drawFastVLine(178, 0, 20, TFT_WHITE);
  //tft.setCursor(145, 21); tft.setTextColor(TFT_WHITE); tft.setTextSize(1); tft.print("Channel:"); tft.print(set_channel);

  if (lnd_an) {

    key[6].initButton(&tft, // Exit box
                      137,
                      10, // x, y, w, h, outline, fill, text
                      EXT_BUTTON_WIDTH,
                      EXT_BUTTON_WIDTH,
                      TFT_ORANGE, // Outline
                      TFT_RED, // Fill
                      TFT_BLACK, // Text
                      "X",
                      2);
  }

  else {
    key[6].initButton(&tft, // Exit box
                      EXT_BUTTON_WIDTH / 2,
                      (STATUS_BAR_WIDTH * 2) + CHAR_WIDTH - 1, // x, y, w, h, outline, fill, text
                      EXT_BUTTON_WIDTH,
                      EXT_BUTTON_WIDTH,
                      TFT_ORANGE, // Outline
                      TFT_RED, // Fill
                      TFT_BLACK, // Text
                      "X",
                      2);
  }

  key[6].setLabelDatum(1, 5, MC_DATUM);

  key[6].drawButton();
}

void Display::twoPartDisplay(String center_text)
{
  tft.setTextColor(TFT_BLACK, TFT_YELLOW);
  tft.fillRect(0,16,HEIGHT_1,144, TFT_YELLOW);
  //tft.drawCentreString(center_text,120,82,1);
  tft.setTextWrap(true);
  tft.setFreeFont(NULL);
  //showCenterText(center_text, 82);
  //tft.drawCentreString(center_text,120,82,1);
  tft.setCursor(0, 82);
  tft.println(center_text);
  tft.setFreeFont(MENU_FONT);
  tft.setTextWrap(false);
}

void Display::touchToExit()
{
  tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
  tft.fillRect(0,32,HEIGHT_1,16, TFT_LIGHTGREY);
  tft.drawCentreString(text11,120,32,2);
}


// Function to just draw the screen black
void Display::clearScreen()
{
  //Serial.println(F("clearScreen()"));
  #ifndef MARAUDER_V7
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
  #else
    tft.fillRect(0, 0, TFT_WIDTH, TFT_HEIGHT, TFT_BLACK);
    tft.setCursor(0, 0);
  #endif
}

#ifdef SCREEN_BUFFER
void Display::scrollScreenBuffer(bool down) {
  // Scroll screen normal direction (Up)
  if (!down) {
    this->screen_buffer->shift();
  }
}
#endif

void Display::processAndPrintString(TFT_eSPI& tft, const String& originalString) {
  // Define colors
  uint16_t text_color = TFT_GREEN; // Default text color
  uint16_t background_color = TFT_BLACK; // Default background color

  String new_string = originalString;

  // Check for color macros at the start of the string
  if (new_string.startsWith(RED_KEY)) {
    text_color = TFT_RED;
    new_string.remove(0, strlen(RED_KEY)); // Remove the macro
  } else if (new_string.startsWith(GREEN_KEY)) {
    text_color = TFT_GREEN;
    new_string.remove(0, strlen(GREEN_KEY)); // Remove the macro
  } else if (new_string.startsWith(CYAN_KEY)) {
    text_color = TFT_CYAN;
    new_string.remove(0, strlen(CYAN_KEY)); // Remove the macro
  } else if (new_string.startsWith(WHITE_KEY)) {
    text_color = TFT_WHITE;
    new_string.remove(0, strlen(WHITE_KEY)); // Remove the macro
  } else if (new_string.startsWith(MAGENTA_KEY)) {
    text_color = TFT_MAGENTA;
    new_string.remove(0, strlen(MAGENTA_KEY)); // Remove the macro
  }

  // Set text color and print the string
  tft.setTextColor(text_color, background_color);
  tft.print(new_string);
}

void Display::displayBuffer(bool do_clear)
{
  if (this->display_buffer->size() > 0)
  {
    int print_count = 1;
    while ((display_buffer->size() > 0) && (print_count > 0))
    {

      #ifndef SCREEN_BUFFER
        xPos = 0;
        if ((display_buffer->size() > 0) && (!loading))
        {
          printing = true;
          delay(print_delay_1);
          yDraw = scroll_line(TFT_RED);
          tft.setCursor(xPos, yDraw);
          tft.setTextColor(TFT_GREEN, TFT_BLACK);
          tft.print(display_buffer->shift());
          printing = false;
          delay(print_delay_2);
        }
        if (!tteBar)
          blank[(18+(yStart - TOP_FIXED_AREA) / TEXT_HEIGHT)%19] = xPos;
        else
          blank[(18+(yStart - TOP_FIXED_AREA_2) / TEXT_HEIGHT)%19] = xPos;
      #else
        xPos = 0;
        if (this->screen_buffer->size() >= MAX_SCREEN_BUFFER)
          this->scrollScreenBuffer();

        screen_buffer->add(display_buffer->shift());

        for (int i = 0; i < this->screen_buffer->size(); i++) {
          tft.setCursor(xPos, (i * 12) + (SCREEN_HEIGHT / 6));
          String spaces = String(' ', TFT_WIDTH / CHAR_WIDTH);
          tft.print(spaces);
          tft.setCursor(xPos, (i * 12) + (SCREEN_HEIGHT / 6));

          this->processAndPrintString(tft, this->screen_buffer->get(i));
        }
      #endif

      print_count--;
    }
  }
}

void Display::showCenterText(String text, int y)
{
  tft.setCursor((SCREEN_WIDTH - (text.length() * (6 * BANNER_TEXT_SIZE))) / 2, y);
  tft.println(text);
}


void Display::initScrollValues(bool tte)
{
  //Serial.println(F("initScrollValues()"));
  yDraw = YMAX - BOT_FIXED_AREA - TEXT_HEIGHT;

  xPos = 0;

  if (!tte)
  {
    yStart = TOP_FIXED_AREA;

    yArea = YMAX - TOP_FIXED_AREA - BOT_FIXED_AREA;
  }
  else
  {
    yStart = TOP_FIXED_AREA_2;

    yArea = YMAX - TOP_FIXED_AREA_2 - BOT_FIXED_AREA;
  }

  for(uint8_t i = 0; i < 18; i++) blank[i] = 0;
}



// Function to execute hardware scroll for TFT screen
int Display::scroll_line(uint32_t color) {
  //Serial.println("scroll_line()");
  int yTemp = yStart; // Store the old yStart, this is where we draw the next line
  // Use the record of line lengths to optimise the rectangle size we need to erase the top line

  // Check if we have the "touch to exit bar"
  if (!tteBar)
  {
    tft.fillRect(0,yStart,blank[(yStart-TOP_FIXED_AREA)/TEXT_HEIGHT],TEXT_HEIGHT, color);
  
    // Change the top of the scroll area
    yStart+=TEXT_HEIGHT;
    // The value must wrap around as the screen memory is a circular buffer
    if (yStart >= YMAX - BOT_FIXED_AREA) yStart = TOP_FIXED_AREA + (yStart - YMAX + BOT_FIXED_AREA);
  }
  else
  {
    tft.fillRect(0,yStart,blank[(yStart-TOP_FIXED_AREA_2)/TEXT_HEIGHT],TEXT_HEIGHT, color);
  
    // Change the top of the scroll area
    yStart+=TEXT_HEIGHT;
    // The value must wrap around as the screen memory is a circular buffer
    if (yStart >= YMAX - BOT_FIXED_AREA) yStart = TOP_FIXED_AREA_2 + (yStart - YMAX + BOT_FIXED_AREA);
  }
  // Now we can scroll the display
  scrollAddress(yStart);
  return  yTemp;
}


// Function to setup hardware scroll for TFT screen
void Display::setupScrollArea(uint16_t tfa, uint16_t bfa) {
  //Serial.println(F("setupScrollArea()"));
  //Serial.println("   tfa: " + (String)tfa);
  //Serial.println("   bfa: " + (String)bfa);
  //Serial.println("yStart: " + (String)this->yStart);
  #ifdef HAS_ILI9341
    #ifdef HAS_ST7789
      tft.writecommand(ST7789_VSCRDEF); // Vertical scroll definition
    #else
      tft.writecommand(ILI9341_VSCRDEF);
    #endif
    tft.writedata(tfa >> 8);           // Top Fixed Area line count
    tft.writedata(tfa);
    tft.writedata((YMAX-tfa-bfa)>>8);  // Vertical Scrolling Area line count
    tft.writedata(YMAX-tfa-bfa);
    tft.writedata(bfa >> 8);           // Bottom Fixed Area line count
    tft.writedata(bfa);
  #endif
}


void Display::scrollAddress(uint16_t vsp) {
  #ifdef HAS_ILI9341
    #ifdef HAS_ST7789
      tft.writecommand(ST7789_VSCRDEF); // Vertical scroll definition
    #else
      tft.writecommand(ILI9341_VSCRDEF);
    #endif
    tft.writedata(vsp>>8);
    tft.writedata(vsp);
  #endif
}




// JPEG_functions
/*
void Display::drawJpeg(const char *filename, int xpos, int ypos) {

  // Open the named file (the Jpeg decoder library will close it after rendering image)
  //fs::File jpegFile = SPIFFS.open( filename, "r");    // File handle reference for SPIFFS

  //jpegFile.close();

  //ESP32 always seems to return 1 for jpegFile so this null trap does not work
  //if ( !jpegFile ) {
  //  Serial.print("ERROR: File \""); Serial.print(filename); Serial.println ("\" not found!");
  //  return;
  //}

  // Use one of the three following methods to initialise the decoder,
  // the filename can be a String or character array type:

  //boolean decoded = JpegDec.decodeFsFile(filename);  // or pass the filename (leading / distinguishes SPIFFS files)
  boolean decoded = JpegDec.decodeArray(MarauderTitle, MARAUDER_TITLE_BYTES);

  if (decoded) {
    // print information about the image to the serial port
    jpegInfo();

    // render the image onto the screen at given coordinates
    jpegRender(xpos, ypos);
  }
  //else {
  //  Serial.println(F("Jpeg file format not supported!"));
  //}
}
*/

/*void Display::setupDraw() {
  this->tft.drawLine(0, 0, 10, 0, TFT_MAGENTA);
  this->tft.drawLine(0, 0, 0, 10, TFT_GREEN);
  this->tft.drawLine(0, 0, 0, 0, TFT_CYAN);
}*/

/*uint16_t xlast;
uint16_t ylast;
uint32_t AH;
void Display::drawStylus()
{
  uint16_t x = 0, y = 0; // To store the touch coordinates

  // Pressed will be set true is there is a valid touch on the screen
  boolean pressed = tft.getTouch(&x, &y);

  if ((x <= 10) && (y <= 10) && (pressed)) {
    //Serial.println(F("Exit draw function"));
    this->draw_tft = false;
    this->exit_draw = true;
    return;
  }

  // Draw a white spot at the detected coordinates
  if (pressed) {
    //    tft.fillCircle(x, y, 2, TFT_WHITE);
    if ( xlast > 0 && ylast > 0 ) {
      uint16_t the_color = TFT_WHITE;
      uint16_t wd = 1;
      int xlast2;
      int ylast2;
      int x2;
      int y2;
      int n;
      int n2 = -wd;
      xlast2 = xlast - wd;
      x2 = x - wd;
      for (n = -wd; n <= wd; n++) {
        ylast2 = ylast + n;
        y2 = y + n;
        tft.drawLine(xlast2, ylast2, x2, y2, the_color);
      }
      for (n2 = -wd; n2 <= wd; n2++) {
        xlast2 = xlast + n2;
        x2 = x + n2;
        tft.drawLine(xlast2, ylast2, x2, y2, the_color);
      }
      for (n = wd; n >= -wd; n--) {
        ylast2 = ylast + n;
        y2 = y + n;
        tft.drawLine(xlast2, ylast2, x2, y2, the_color);
      }
      for (n2 = wd; n2 >= -wd; n2--) {
        xlast2 = xlast + n2;
        x2 = x + n2;
        tft.drawLine(xlast2, ylast2, x2, y2, the_color);
      }
//      tft.drawLine(xlast, ylast, x, y, TFT_WHITE);
    }
    xlast = x;
    ylast = y;
    AH = 0;
    //Serial.print("x,y = ");
    //Serial.print(x);
    //Serial.print(",");
    //Serial.println(y);
  } else if ( AH < 5 ) {
    AH++;
  } else if ( AH == 5 ) {
    xlast = 0;
    ylast = 0;
  }
}*/

//====================================================================================
//   Decode and render the Jpeg image onto the TFT screen
//====================================================================================
/*void Display::jpegRender(int xpos, int ypos) {

  // retrieve infomration about the image
  uint16_t  *pImg;
  int16_t mcu_w = JpegDec.MCUWidth;
  int16_t mcu_h = JpegDec.MCUHeight;
  int32_t max_x = JpegDec.width;
  int32_t max_y = JpegDec.height;

  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  int32_t min_w = minimum(mcu_w, max_x % mcu_w);
  int32_t min_h = minimum(mcu_h, max_y % mcu_h);

  // save the current image block size
  int32_t win_w = mcu_w;
  int32_t win_h = mcu_h;

  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();

  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;

  // read each MCU block until there are no more
  while ( JpegDec.readSwappedBytes()) { // Swapped byte order read

    // save a pointer to the image block
    pImg = JpegDec.pImage;

    // calculate where the image block should be drawn on the screen
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;  // Calculate coordinates of top left corner of current MCU
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;

    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      for (int h = 1; h < win_h-1; h++)
      {
        memcpy(pImg + h * win_w, pImg + (h + 1) * mcu_w, win_w << 1);
      }
    }

    // draw image MCU block only if it will fit on the screen
    if ( mcu_x < tft.width() && mcu_y < tft.height())
    {
      // Now push the image block to the screen
      tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
    }

    else if ( ( mcu_y + win_h) >= tft.height()) JpegDec.abort();

  }

  // calculate how long it took to draw the image
  drawTime = millis() - drawTime; // Calculate the time it took
}*/

//====================================================================================
//   Print information decoded from the Jpeg image
//====================================================================================
/*void Display::jpegInfo() {

  Serial.println("===============");
  Serial.println("JPEG image info");
  Serial.println("===============");
  Serial.print  ("Width      :"); Serial.println(JpegDec.width);
  Serial.print  ("Height     :"); Serial.println(JpegDec.height);
  Serial.print  ("Components :"); Serial.println(JpegDec.comps);
  Serial.print  ("MCU / row  :"); Serial.println(JpegDec.MCUSPerRow);
  Serial.print  ("MCU / col  :"); Serial.println(JpegDec.MCUSPerCol);
  Serial.print  ("Scan type  :"); Serial.println(JpegDec.scanType);
  Serial.print  ("MCU width  :"); Serial.println(JpegDec.MCUWidth);
  Serial.print  ("MCU height :"); Serial.println(JpegDec.MCUHeight);
  Serial.println("===============");
  Serial.println("");
}*/

//====================================================================================
//   Open a Jpeg file and send it to the Serial port in a C array compatible format
//====================================================================================
/*void createArray(const char *filename) {

  // Open the named file
  fs::File jpgFile = SPIFFS.open( filename, "r");    // File handle reference for SPIFFS
  //  File jpgFile = SD.open( filename, FILE_READ);  // or, file handle reference for SD library

  if ( !jpgFile ) {
    Serial.print("ERROR: File \""); Serial.print(filename); Serial.println ("\" not found!");
    return;
  }

  uint8_t data;
  byte line_len = 0;
  Serial.println("");
  Serial.println(F("// Generated by a JPEGDecoder library example sketch:"));
  Serial.println(F("// https://github.com/Bodmer/JPEGDecoder"));
  Serial.println("");
  Serial.println(F("#if defined(__AVR__)"));
  Serial.println(F("  #include <avr/pgmspace.h>"));
  Serial.println(F("#endif"));
  Serial.println("");
  Serial.print  (F("const uint8_t "));
  while (*filename != '.') Serial.print(*filename++);
  Serial.println(F("[] PROGMEM = {")); // PROGMEM added for AVR processors, it is ignored by Due

  while ( jpgFile.available()) {

    data = jpgFile.read();
    Serial.print("0x"); if (abs(data) < 16) Serial.print("0");
    Serial.print(data, HEX); Serial.print(",");// Add value and comma
    line_len++;
    if ( line_len >= 32) {
      line_len = 0;
      Serial.println();
    }

  }

  Serial.println("};\r\n");
  jpgFile.close();
}*/

// End JPEG_functions

// SPIFFS_functions

#ifdef ESP8266
void Display::listFiles(void) {
  Serial.println();
  Serial.println(F("SPIFFS files found:"));

  fs::Dir dir = SPIFFS.openDir("/"); // Root directory
  String  line = "=====================================";

  Serial.println(line);
  Serial.println(F("  File name               Size"));
  Serial.println(line);

  while (dir.next()) {
    String fileName = dir.fileName();
    Serial.print(fileName);
    int spaces = 21 - fileName.length(); // Tabulate nicely
    while (spaces--) Serial.print(" ");

    fs::File f = dir.openFile("r");
    String fileSize = (String) f.size();
    spaces = 10 - fileSize.length(); // Tabulate nicely
    while (spaces--) Serial.print(" ");
    Serial.println(fileSize + " bytes");
  }

  Serial.println(line);
  Serial.println();
  delay(1000);
}
#endif

//====================================================================================

#ifdef ESP32

void Display::listFiles(void) {
  listDir(SPIFFS, "/", 0);
}

void Display::listDir(fs::FS &fs, const char * dirname, uint8_t levels) {

  Serial.println();
  Serial.println(F("SPIFFS files found:"));

  Serial.printf("Listing directory: %s\n", "/");
  String  line = "=====================================";

  Serial.println(line);
  Serial.println(F("  File name               Size"));
  Serial.println(line);

  fs::File root = fs.open(dirname);
  if (!root) {
    Serial.println(F("Failed to open directory"));
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(F("Not a directory"));
    return;
  }

  fs::File file = root.openNextFile();
  while (file) {

    if (file.isDirectory()) {
      Serial.print("DIR : ");
      String fileName = file.name();
      Serial.print(fileName);
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      String fileName = file.name();
      Serial.print("  " + fileName);
      int spaces = 20 - fileName.length(); // Tabulate nicely
      while (spaces--) Serial.print(" ");
      String fileSize = (String) file.size();
      spaces = 10 - fileSize.length(); // Tabulate nicely
      while (spaces--) Serial.print(" ");
      Serial.println(fileSize + " bytes");
    }

    file = root.openNextFile();
  }

  Serial.println(line);
  Serial.println();
  delay(1000);
}
#endif


void Display::updateBanner(String msg)
{
  //this->img.deleteSprite();
  
  //this->img.setColorDepth(8);

  //this->img.createSprite(SCREEN_WIDTH, TEXT_HEIGHT);

  this->buildBanner(msg, current_banner_pos);

  //this->img.pushSprite(0, STATUS_BAR_WIDTH);

  //current_banner_pos--;

  //if (current_banner_pos <= 0)
  //  current_banner_pos = SCREEN_WIDTH + 2;
}


void Display::buildBanner(String msg, int xpos)
{
  int h = TEXT_HEIGHT;

  this->tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, TEXT_HEIGHT, TFT_BLACK);
  this->tft.setFreeFont(NULL);           // Font 4 selected
  this->tft.setTextSize(BANNER_TEXT_SIZE);           // Font size scaling is x1
  this->tft.setTextColor(TFT_WHITE, TFT_BLACK);  // Black text, no background colour
  this->showCenterText(msg, STATUS_BAR_WIDTH);

  /*
  // We could just use fillSprite(color) but lets be a bit more creative...

  // Fill with rainbow stripes
  //while (h--) img.drawFastHLine(0, h, SCREEN_WIDTH, 255);

  // Draw some graphics, the text will apear to scroll over these
  //img.fillRect  (SCREEN_WIDTH / 2 - 20, TEXT_HEIGHT / 2 - 10, 40, 20, TFT_YELLOW);
  //img.fillCircle(SCREEN_WIDTH / 2, TEXT_HEIGHT / 2, 10, TFT_ORANGE);

  // Now print text on top of the graphics
  img.setTextSize(BANNER_TEXT_SIZE);           // Font size scaling is x1
  img.setTextFont(0);           // Font 4 selected
  img.setTextColor(TFT_WHITE);  // Black text, no background colour
  img.setTextWrap(false);       // Turn of wrap so we can print past end of sprite

  // Need to print twice so text appears to wrap around at left and right edges
  img.setCursor(xpos, 2);  // Print text at xpos
  img.print(msg);

  img.setCursor(xpos - SCREEN_WIDTH, 2); // Print text at xpos - sprite width
  img.print(msg);
  */
}

void Display::main(uint8_t scan_mode)
{  
  if ((scan_mode == LV_JOIN_WIFI) ||
      (scan_mode == LV_ADD_SSID))
    lv_task_handler();
  return;
}
// End SPIFFS_functions

#endif
