// HelloWorld for the Pervasive Displays SE2266JS0C5 (2.66" BWR, 296x152), as found in the
// SES-imagotag / VUSION 2.6 BWR GU110 shelf label. Driver: GxEPD2_266c_SE2266JS0C5.
// Board: ESP32-C3 Super Mini.
// Wiring: CS=GPIO10, DC=GPIO5, RST=GPIO3, BUSY=GPIO1, SCLK=GPIO6, MOSI=GPIO7 (MISO unused),
//         ON/OFF (panel power) = GPIO4.  Keep panel signals OFF the strapping pins GPIO9/GPIO2 - see README.
//
// EPD_PWR (GPIO4) drives the VUSION tag board's ON-BOARD power MOSFET (via its ON/OFF test point) -
// no external MOSFET needed. On a bare panel breakout with no switch, leave EPD_PWR unconnected (the
// writes below are then harmless). The panel is bistable: once refreshed, the image holds with the
// supply cut, so the tag draws ~zero current between updates.

#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_3C.h>
#include "GxEPD2_266c_SE2266JS0C5.h"

#define EPD_CS   10
#define EPD_DC   5  // NOT GPIO9 (that is the BOOT strapping pin - see README)
#define EPD_RST  3
#define EPD_BUSY 1  // NOT GPIO2 (strapping pin)
#define EPD_SCLK 6
#define EPD_MISO -1 // unused (panel is write-only)
#define EPD_MOSI 7
#define EPD_PWR  4  // ON/OFF -> tag board's on-board power MOSFET

// The VUSION board's MOSFET is active-low: LOW = panel powered.
#define PWR_ON   LOW
#define PWR_OFF  HIGH

GxEPD2_3C<GxEPD2_266c_SE2266JS0C5, GxEPD2_266c_SE2266JS0C5::HEIGHT> display(
  GxEPD2_266c_SE2266JS0C5(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== SE2266JS0C5 3C HelloWorld ===");

  // power the panel on before we talk to it (150 ms for the boost to settle)
  pinMode(EPD_PWR, OUTPUT);
  digitalWrite(EPD_PWR, PWR_ON);
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

  Serial.println("Refresh done - powering panel off (image holds).");
  display.powerOff();

  // Park the SPI / control lines LOW so they can't back-power the unpowered panel through its ESD
  // diodes, then cut the panel supply. The image stays on screen at zero current.
  digitalWrite(EPD_CS, LOW);
  digitalWrite(EPD_DC, LOW);
  digitalWrite(EPD_RST, LOW);
  digitalWrite(EPD_SCLK, LOW);
  digitalWrite(EPD_MOSI, LOW);
  digitalWrite(EPD_PWR, PWR_OFF); // panel OFF

  Serial.println("=== Done ===");
}

void loop()
{
  delay(1000);
}
