// HelloWorld for the Pervasive Displays E2266JS0C (2.66" BWR, 296x152), as found in the
// SES-imagotag / VUSION 2.6 BWR GU110 shelf label. Driver: GxEPD2_266c_E2266JS0C.
// Board: ESP32-C3 Super Mini (same wiring as the 5.81 example).
// Wiring: CS=GPIO10, DC=GPIO5, RST=GPIO3, BUSY=GPIO1, SCLK=GPIO6, MOSI=GPIO7 (MISO unused),
//         ON/OFF (panel power) = GPIO4.  Keep panel signals OFF the strapping pins GPIO9/GPIO2 - see README.

#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_3C.h>
#include "GxEPD2_266c_E2266JS0C.h"

#define EPD_CS   10
#define EPD_DC   5  // NOT GPIO9 (that is the BOOT strapping pin - see README)
#define EPD_RST  3
#define EPD_BUSY 1  // NOT GPIO2 (strapping pin)
#define EPD_SCLK 6
#define EPD_MISO -1 // unused (panel is write-only)
#define EPD_MOSI 7
#define EPD_PWR  4  // panel power (LOW = ON); harmless if your board has no power switch

GxEPD2_3C<GxEPD2_266c_E2266JS0C, GxEPD2_266c_E2266JS0C::HEIGHT> display(
  GxEPD2_266c_E2266JS0C(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== E2266JS0C 3C HelloWorld ===");

  // power the panel on before we talk to it (150 ms for the boost to settle)
  pinMode(EPD_PWR, OUTPUT);
  digitalWrite(EPD_PWR, LOW);
  delay(150);

  SPI.begin(EPD_SCLK, EPD_MISO, EPD_MOSI, EPD_CS);

  display.init(115200);
  display.setRotation(3); // landscape, 296x152

  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setTextSize(2);
    display.setCursor(20, 30);
    display.print("GxEPD2 driver");
    display.setTextColor(GxEPD_RED);
    display.setCursor(20, 70);
    display.print("works!");
    display.drawRect(10, 10, 276, 132, GxEPD_BLACK);
    display.fillCircle(250, 110, 15, GxEPD_RED);
  } while (display.nextPage());

  Serial.println("=== Done ===");
  display.powerOff();
}

void loop()
{
  delay(1000);
}
