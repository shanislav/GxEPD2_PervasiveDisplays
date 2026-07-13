// HelloWorld for the SES-imagotag / VUSION 9.7" BWR panel (TE2969JS0B4), 960x672, dual-COG,
// on a Wemos / LOLIN S2 Mini (ESP32-S2). The 9.7" is the panel that benefits most from the S2's
// larger RAM; the smaller panels run fine on an ESP32-C3 (see the other examples).
//
// S2 wiring (panel on the board's high-GPIO header block):
//   M-CS=GPIO34, S-CS=GPIO33, DC=GPIO37, RST=GPIO38, BUSY(master)=GPIO39, SCLK=GPIO36, MOSI=GPIO35,
//   panel power ON/OFF = GPIO40 (drives the tag's on-board MOSFET, LOW = ON).
// Avoid the S2 strapping pins GPIO0 / GPIO45 / GPIO46 and the flash/PSRAM pins GPIO26-32.
//
// IMPORTANT: do NOT call SPI.begin() in this sketch. This driver reads the panel OTP by bit-banging,
// then calls SPI.begin() itself. On the ESP32-S2, a prior SPI.begin() on the SPI pins permanently
// breaks the bit-bang read-back (it returns 0xFF), so the init comes out garbage and the panel
// powers up but never refreshes. See the README's ESP32-S2 section.

#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_3C.h>
#include "GxEPD2_970c_TE2969JS0B4.h"

#define EPD_CS_M 34  // master CS
#define EPD_CS_S 33  // slave CS
#define EPD_DC   37
#define EPD_RST  38
#define EPD_BUSY 39  // master BUSY
#define EPD_SCLK 36
#define EPD_MOSI 35
#define EPD_PWR  40  // ON/OFF -> tag board's on-board power MOSFET

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
      int inten = ((px - x0) * 17) / w; // 0 (solid cA far left) .. 16 (solid cB far right)
      uint16_t c = (inten > bayer4[py & 3][px & 3]) ? cB : cA;
      display.drawPixel(px, py, c);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== VUSION 9.7 (TE2969JS0B4) on Wemos S2 Mini ===");

  // power the panel ON (FET active-low), let the boost settle
  pinMode(EPD_PWR, OUTPUT);
  digitalWrite(EPD_PWR, PWR_ON);
  delay(200);

  // NOTE: no SPI.begin() here - the driver reads OTP by bit-banging first, then inits SPI itself.
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
    display.print("VUSION 9.7 on S2 Mini");
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
