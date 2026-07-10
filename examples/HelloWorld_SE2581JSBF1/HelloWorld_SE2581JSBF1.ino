// Example for the SES-imagotag / VUSION 5.9" BWR panel (SE2581JSBF1), 720x256, 3-colour,
// on an ESP32-C3 Super Mini. Driver: GxEPD2_581c_SE2581JSBF1 (companion to GxEPD2).
//
// The cycle for a recycled ESL panel: power the panel ON -> draw + refresh -> power it OFF.
// The panel is bistable, so it keeps the image with zero current once the supply is cut.
//
// EPD_PWR (GPIO4) drives the VUSION tag board's ON-BOARD power MOSFET (via its ON/OFF test point) -
// no external MOSFET needed. On a bare panel breakout with no switch, leave EPD_PWR unconnected (the
// digitalWrite() calls are then harmless).
//
// Wiring: CS=GPIO10, DC=GPIO5, RST=GPIO3, BUSY=GPIO1, SCLK=GPIO6, MOSI=GPIO7 (MISO unused),
//         ON/OFF = GPIO4.  Keep panel signals OFF the strapping pins GPIO9/GPIO2 - see README.

#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_3C.h>
#include "GxEPD2_581c_SE2581JSBF1.h"

#define EPD_CS   10
#define EPD_DC   5  // NOT GPIO9 (that is the BOOT strapping pin - see README)
#define EPD_RST  3
#define EPD_BUSY 1  // NOT GPIO2 (strapping pin)
#define EPD_SCLK 6
#define EPD_MISO -1 // unused (panel is write-only)
#define EPD_MOSI 7
#define EPD_PWR  4  // ON/OFF -> tag board's on-board power MOSFET

// Panel power polarity. On the VUSION tag board the ON/OFF line is active-low (LOW = panel ON).
// Flip these if your board / switch is wired the other way.
#define PWR_ON   LOW
#define PWR_OFF  HIGH

GxEPD2_3C<GxEPD2_581c_SE2581JSBF1, GxEPD2_581c_SE2581JSBF1::HEIGHT> display(
  GxEPD2_581c_SE2581JSBF1(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

void setup()
{
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== VUSION 5.9 (SE2581JSBF1) ===");

  // Power the panel ON right before we use it, then let the boost bring up the rails.
  pinMode(EPD_PWR, OUTPUT);
  digitalWrite(EPD_PWR, PWR_ON);
  delay(150);

  SPI.begin(EPD_SCLK, EPD_MISO, EPD_MOSI, EPD_CS);
  display.init(115200, true, 2, false);
  display.setRotation(0);

  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.drawRect(4, 4, 712, 248, GxEPD_BLACK);
    display.setTextSize(3);
    display.setTextColor(GxEPD_RED);
    display.setCursor(30, 60);
    display.print("VUSION 5.9  -  BWR");
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(30, 120);
    display.print("Panel: SE2581JSBF1  720x256");
    display.setTextColor(GxEPD_RED);
    display.setCursor(30, 175);
    display.print("GxEPD2 driver");
  } while (display.nextPage());

  Serial.println("Refresh done - cutting panel supply (image holds).");

  // Note: no display.powerOff() here. On this COG the soft power-off (0x02) fades the fresh image
  // even after the refresh has fully completed (see README), so we only cut the supply.
  // Park the SPI / control lines LOW so they can't back-power the unpowered panel through its ESD
  // diodes, then cut the panel supply. The image stays on screen at zero current.
  digitalWrite(EPD_CS, LOW);
  digitalWrite(EPD_DC, LOW);
  digitalWrite(EPD_RST, LOW);
  digitalWrite(EPD_SCLK, LOW);
  digitalWrite(EPD_MOSI, LOW);
  digitalWrite(EPD_PWR, PWR_OFF); // panel OFF

  Serial.println("=== Done - image holds with the panel unpowered ===");
}

void loop()
{
  delay(2000);
}
