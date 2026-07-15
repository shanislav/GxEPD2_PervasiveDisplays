// Example for the SES-imagotag / VUSION 9.7" BWR panel (TE2969JS0B4), 960x672, 3-colour, DUAL-COG,
// on an ESP32-C3 Super Mini. Driver: GxEPD2_970c_TE2969JS0B4 (companion to GxEPD2).
//
// This panel has TWO controllers (master + slave), each driving half the glass. They share
// DC/RST/SDA/SCL and are picked by separate CS lines. Wiring:
//   M-CS=GPIO10, S-CS=GPIO0, DC=GPIO5, RST=GPIO3, BUSY(master)=GPIO1, SCLK=GPIO6, MOSI=GPIO7,
//   panel power ON/OFF = GPIO4 (drives the tag's on-board MOSFET, LOW = ON).
// Keep panel signals off the C3 strapping pins GPIO9/GPIO2 - see the README.
//
// Memory: the two 80 KB frame buffers are malloc'd on the heap and the GFX page buffer is HEIGHT/4,
// so it fits the ESP32-C3 RAM (no WiFi). The driver reads the panel's OTP (command 0xB9) for its
// per-panel init parameters, so no waveform tables are hardcoded.

#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_3C.h>
#include "GxEPD2_970c_TE2969JS0B4.h"

#define EPD_CS_M 10  // master CS
#define EPD_CS_S 0   // slave CS
#define EPD_DC   5   // NOT GPIO9 (BOOT strapping pin - see README)
#define EPD_RST  3
#define EPD_BUSY 1   // master BUSY; NOT GPIO2 (strapping pin)
#define EPD_SCLK 6
#define EPD_MOSI 7
#define EPD_PWR  4   // ON/OFF -> tag board's on-board power MOSFET

// The VUSION board's MOSFET is active-low: LOW = panel powered.
#define PWR_ON   LOW
#define PWR_OFF  HIGH

GxEPD2_3C<GxEPD2_970c_TE2969JS0B4, GxEPD2_970c_TE2969JS0B4::HEIGHT / 4> display(
  GxEPD2_970c_TE2969JS0B4(EPD_CS_M, EPD_CS_S, EPD_DC, EPD_RST, EPD_BUSY, EPD_SCLK, EPD_MOSI));

// 4x4 ordered (Bayer) dither matrix, values 0..15
static const uint8_t bayer4[4][4] = {
  { 0,  8,  2, 10},
  {12,  4, 14,  6},
  { 3, 11,  1,  9},
  {15,  7, 13,  5}
};

// Horizontal dithered gradient from colour cA (left) to cB (right) within [x0, x0+w).
static void gradientStripe(int x0, int y0, int w, int h, uint16_t cA, uint16_t cB)
{
  for (int py = y0; py < y0 + h; py++)
  {
    for (int px = x0; px < x0 + w; px++)
    {
      int inten = ((px - x0) * 17) / w; // 0 (solid cA at far left) .. 16 (solid cB at far right)
      uint16_t c = (inten > bayer4[py & 3][px & 3]) ? cB : cA;
      display.drawPixel(px, py, c);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== VUSION 9.7 (TE2969JS0B4) dual-COG 3C ===");

  // power the panel ON (FET active-low), let the boost settle
  pinMode(EPD_PWR, OUTPUT);
  digitalWrite(EPD_PWR, PWR_ON);
  delay(200);

  // ESP32-C3: SPI.begin() before init() is required here - without it, the OTP bit-bang read below
  // comes back all 0xFF. (On the ESP32-S2 it is the opposite - see HelloWorld_TE2969JS0B4_S2mini for
  // that board's OTP-reading panels.)
  SPI.begin(EPD_SCLK, -1, EPD_MOSI, EPD_CS_M);
  display.init(115200, true, 2, false);
  display.setRotation(0);

  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.drawRect(4, 4, 952, 664, GxEPD_BLACK);
    display.setTextSize(4);
    display.setTextColor(GxEPD_RED);
    display.setCursor(40, 50);
    display.print("VUSION 9.7  -  BWR");
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(40, 130);
    display.print("Panel: TE2969JS0B4");
    display.setCursor(40, 190);
    display.print("960 x 672  dual-COG");
    display.setTextColor(GxEPD_RED);
    display.setCursor(40, 270);
    display.print("GxEPD2 driver");

    // three dithered gradient stripes (each solid colour at both ends)
    gradientStripe(30, 360, 900, 80, GxEPD_WHITE, GxEPD_BLACK); // white -> black
    gradientStripe(30, 450, 900, 80, GxEPD_WHITE, GxEPD_RED);   // white -> red
    gradientStripe(30, 540, 900, 80, GxEPD_BLACK, GxEPD_RED);   // black -> red
  } while (display.nextPage());

  Serial.println("Refresh done - powering panel off (image holds).");
  display.powerOff();

  // Park the SPI / control lines LOW so they can't back-power the unpowered panel through its ESD
  // diodes, then cut the panel supply. The image stays on screen at zero current.
  digitalWrite(EPD_CS_M, LOW);
  digitalWrite(EPD_CS_S, LOW);
  digitalWrite(EPD_DC, LOW);
  digitalWrite(EPD_RST, LOW);
  digitalWrite(EPD_SCLK, LOW);
  digitalWrite(EPD_MOSI, LOW);
  digitalWrite(EPD_PWR, PWR_OFF); // panel OFF

  Serial.println("=== done ===");
}

void loop()
{
  delay(2000);
}
