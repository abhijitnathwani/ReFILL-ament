/*
Test Code for OLED Display for the ReFILL-ament project.
https://www.hackster.io/abhijit-nathwani/refill-ament-for-3d-printers-261881
By Abhijit Nathwani
*/
// Include the correct display library
// For a connection via I2C using Wire include
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SH1106.h" //alis for `#include "SH1106Wire.h"`

// Include custom images
#include "images.h"

// Initialize the OLED display using brzo_i2c
// D4 -> SDA
// D3 -> SCL
SH1106 display(0x3c, D4, D3);


#define DEMO_DURATION 3000
typedef void (*Demo)(void);

int demoMode = 0;
int counter = 1;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();


  // Initialising the UI will init the display too.
  display.init();

  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

}

void drawFontFaceDemo() {
    // Font Demo1
    // create more fonts at http://oleddisplay.squix.ch/
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "Hello world");
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 10, "Hello world");
    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 26, "Hello world");
}

void drawTextFlowDemo() {
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawStringMaxWidth(0, 0, 128,
      "Lorem ipsum\n dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore." );
}

void drawTextAlignmentDemo() {
    // Text alignment demo
  display.setFont(ArialMT_Plain_10);

  // The coordinates define the left starting point of the text
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 10, "Left aligned (0,10)");

  // The coordinates define the center of the text
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 22, "Center aligned (64,22)");

  // The coordinates define the right end of the text
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(128, 33, "Right aligned (128,33)");
}

void drawRectDemo() {
      // Draw a pixel at given position
    for (int i = 0; i < 10; i++) {
      display.setPixel(i, i);
      display.setPixel(10 - i, i);
    }
    display.drawRect(12, 12, 20, 20);

    // Fill the rectangle
    display.fillRect(14, 14, 17, 17);

    // Draw a line horizontally
    display.drawHorizontalLine(0, 40, 20);

    // Draw a line horizontally
    display.drawVerticalLine(40, 0, 20);
}

void drawCircleDemo() {
  for (int i=1; i < 8; i++) {
    display.setColor(WHITE);
    display.drawCircle(32, 32, i*3);
    if (i % 2 == 0) {
      display.setColor(BLACK);
    }
    display.fillCircle(96, 32, 32 - i* 3);
  }
}

void drawProgressBarDemo() {
  int progress = (counter / 5) % 100;
  // draw the progress bar
  display.drawProgressBar(0, 32, 120, 10, progress);

  // draw the percentage as String
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 15, String(progress) + "%");
}

void drawImageDemo() {
    // see http://blog.squix.org/2015/05/esp8266-nodemcu-how-to-create-xbm.html
    // on how to create xbm files
//    display.drawXbm(34, 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
}

Demo demos[] = {drawFontFaceDemo, drawTextFlowDemo, drawTextAlignmentDemo, drawRectDemo, drawCircleDemo, drawProgressBarDemo, drawImageDemo};
int demoLength = (sizeof(demos) / sizeof(Demo));
long timeSinceLastModeSwitch = 0;

void loop() {
  // clear the display
  display.clear();
  display.drawXbm(0, 0, rfl_scr_w, rfl_scr_h, rfl_scr);
  display.display();
  delay(5000);

  display.clear();
  display.drawXbm(0, 0, drs_scr_w, drs_scr_h, drs_scr);
  display.display();
  delay(5000);

  display.clear();
  display.drawXbm(0, 0, ccn_scr_w, ccn_scr_h, ccn_scr);
  display.display();
  delay(5000);

  display.clear();
  display.drawXbm(0, 0, cls_scr_w, cls_scr_h, cls_scr);
  display.display();
  delay(5000);

  display.clear();
  display.drawXbm(0, 0, aps_scr_w, aps_scr_h, aps_scr);
  display.display();
  delay(5000);

  display.clear();
  display.drawXbm(0, 0, nw_img_w, nw_img_h, nw_img);
  display.display();
  delay(5000);

  display.clear();
  display.drawXbm(0, 0, nonw_img_w, nonw_img_h, nonw_img);
  display.display();
  delay(5000);
  // draw the current demo method
  // The coordinates define the center of the text
  //display.setFont(ArialMT_Plain_16);
  //display.setTextAlignment(TEXT_ALIGN_CENTER);
  //display.drawString(64, 22, "Center aligned (64,22)");
  //display.display();
  //delay(1000);
  //display.drawLine(0, 16, 128,16);
  //display.display();
  //delay(1000);
}
