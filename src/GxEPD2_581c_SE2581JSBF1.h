// Display Library for Pervasive Displays SPI e-paper panels (iTC, film J "Spectra" BWR).
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// Driver for the Pervasive Displays 5.81" BWR panel used in SES-imagotag / VUSION 5.9 GU110
// electronic shelf labels (EDG3-0590-A). The panel reports itself in OTP as "SE2581JSBF1".
// Physical resolution is 720x256, 3-colour (black / white / red).
//
// This is an OEM panel with NO public datasheet or driver. The controller is a GD7965 / UC8179
// (command set 0x00/0x01/0x04/0x10/0x13/0x12/0x50/0x60/0x61) - NOT the IST7236 "medium" COG of
// the public E2581JS0B (Pervasive's EPD_Driver_GU_mid, which drives via 0x15, is a no-op here).
// The parameters below were reverse-engineered empirically. Key findings:
//   * HEIGHT is addressed as 512 (not 256): the GD7965 RAM spans 720x512 while only the top 256
//     rows are wired to the glass. Each plane is 46080 bytes; sending only 23040 left half the
//     panel undriven. Draw in Y = 0..255; anything at Y > 255 lands in RAM but is not visible.
//   * Panel setting (0x00) = 0x0E : the usual 3C-from-OTP value 0x0F hangs this controller.
//   * VCOM/data-interval (0x50) = 0x01,0x07 : 0x01 (not 0x11) keeps the border waveform right
//     without setting the data-polarity bit, which would invert the colours.
//   * powerOff() (command 0x02) is intentionally disabled: it fades the fresh image even after a
//     fully completed refresh (verified on hardware). Cut the panel supply instead.
//
// Panel product page (public E2581 variant): https://www.pervasivedisplays.com/product/5-81-e-ink-displays/
// See PROGRESS.md in this repo for the full reverse-engineering story.
//
// Author: Shano (with Claude), in the style of GxEPD2 by Jean-Marc Zingg
//
// Library: https://github.com/ZinggJM/GxEPD2

#ifndef _GxEPD2_581c_SE2581JSBF1_H_
#define _GxEPD2_581c_SE2581JSBF1_H_

#include <GxEPD2_EPD.h>

class GxEPD2_581c_SE2581JSBF1 : public GxEPD2_EPD
{
  public:
    // attributes
    static const uint16_t WIDTH = 720;
    static const uint16_t WIDTH_VISIBLE = WIDTH;
    static const uint16_t HEIGHT = 512;
    // no dedicated enum entry for Pervasive Displays panels in GxEPD2 (yet);
    // GDEQ0583Z31 is a similar-size 3C panel
    static const GxEPD2::Panel panel = GxEPD2::GDEQ0583Z31;
    static const bool hasColor = true;
    static const bool hasPartialUpdate = false; // COG has no RAM window addressing
    static const bool hasFastPartialUpdate = false;
    static const uint16_t power_on_time = 100; // ms
    static const uint16_t power_off_time = 500; // ms
    static const uint16_t full_refresh_time = 30000; // ms, BWR Spectra full refresh
    static const uint16_t partial_refresh_time = 30000; // ms, no partial mode
    // constructor
    GxEPD2_581c_SE2581JSBF1(int16_t cs, int16_t dc, int16_t rst, int16_t busy);
    // methods (virtual)
    void clearScreen(uint8_t value = 0xFF); // init buffers and screen (default white)
    void clearScreen(uint8_t black_value, uint8_t color_value);
    void writeScreenBuffer(uint8_t value = 0xFF); // init buffers (default white)
    void writeScreenBuffer(uint8_t black_value, uint8_t color_value);
    // write to buffer, without screen refresh; x and w should be multiple of 8
    void writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                        int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void writeImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void writeImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                        int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    // write sprite of native data to buffer, without screen refresh; x and w should be multiple of 8
    void writeNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    // write to buffer, with screen refresh; x and w should be multiple of 8
    void drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void drawImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                       int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void drawImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void drawImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                       int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void drawNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void refresh(bool partial_update_mode = false); // screen refresh from buffer to full screen
    void refresh(int16_t x, int16_t y, int16_t w, int16_t h); // maps to full refresh (no partial mode)
    void powerOff(); // turns off DC/DC of the COG
    void hibernate(); // powerOff, no deep sleep command documented for this COG
  private:
    void _writeImageToBuffer(uint8_t* buffer, const uint8_t* bitmap, int16_t x, int16_t y, int16_t w, int16_t h,
                             bool invert, bool mirror_y, bool pgm);
    void _writeImagePartToBuffer(uint8_t* buffer, const uint8_t* bitmap, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                 int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm);
    void _resetPanel();
    void _InitDisplay();
    void setTemperatureC(int8_t degrees_c);
  private:
    static const uint32_t BUFFER_SIZE = uint32_t(WIDTH) * uint32_t(HEIGHT) / 8; // 720*512/8 = 46080 bytes/plane
    // Allocated on the heap (malloc), not as static member arrays: 2x46 KB of static BSS overflows
    // the ESP32-S2 data RAM at link time. The 9.7"/0G1 drivers do the same.
    uint8_t* _black_buffer;
    uint8_t* _red_buffer;
    int8_t _temperature_c = 25;
};

#endif
