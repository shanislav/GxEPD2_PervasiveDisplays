// Display Library for Pervasive Displays SPI e-paper panels (iTC, film J "Spectra" BWR).
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// Driver for Pervasive Displays 2.66" BWR panel SE2266JS0C5 (SE2266JS0C5), 152x296, iTC driver "0C".
// Command sequence based on Pervasive Displays Application Note (small C/J film panels)
// and reference implementation in PDLS_EXT3_Basic_Global (COG_SmallCJ_* branch).
//
// Panel: https://www.pervasivedisplays.com/product/2-66-e-ink-displays/
// Reference: https://github.com/PervasiveDisplays/PDLS_EXT3_Basic_Global
//
// Note: unlike Good Display/Waveshare controllers, this COG has no partial RAM window
// commands - a full frame must always be sent. This driver therefore keeps its own
// full-frame buffers (2 x 5624 bytes) and transfers them on refresh().
//
// Author: Shano (with Claude), in the style of GxEPD2 by Jean-Marc Zingg
//
// Library: https://github.com/ZinggJM/GxEPD2

#ifndef _GxEPD2_266c_SE2266JS0C5_H_
#define _GxEPD2_266c_SE2266JS0C5_H_

#include <GxEPD2_EPD.h>

class GxEPD2_266c_SE2266JS0C5 : public GxEPD2_EPD
{
  public:
    // attributes
    static const uint16_t WIDTH = 152;
    static const uint16_t WIDTH_VISIBLE = WIDTH;
    static const uint16_t HEIGHT = 296;
    // no dedicated enum entry for Pervasive Displays panels in GxEPD2 (yet);
    // GDEY0266Z90 is the closest match (2.66" BWR, same resolution)
    static const GxEPD2::Panel panel = GxEPD2::GDEY0266Z90;
    static const bool hasColor = true;
    static const bool hasPartialUpdate = false; // COG has no RAM window addressing
    static const bool hasFastPartialUpdate = false;
    static const uint16_t power_on_time = 100; // ms
    static const uint16_t power_off_time = 100; // ms
    static const uint16_t full_refresh_time = 20000; // ms, BWR Spectra full refresh
    static const uint16_t partial_refresh_time = 20000; // ms, no partial mode
    // constructor
    GxEPD2_266c_SE2266JS0C5(int16_t cs, int16_t dc, int16_t rst, int16_t busy);
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
    // set environment temperature used by the COG waveform selection (default 25 degrees C)
    void setTemperatureC(int8_t degrees_c);
  private:
    void _writeImageToBuffer(uint8_t* buffer, const uint8_t* bitmap, int16_t x, int16_t y, int16_t w, int16_t h,
                             bool invert, bool mirror_y, bool pgm);
    void _writeImagePartToBuffer(uint8_t* buffer, const uint8_t* bitmap, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                 int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm);
    void _InitDisplay(); // reset + soft reset + temperature + PSR
    void _Update_Full(); // power on + display refresh + wait
    void _PowerOff(); // DC/DC off
  private:
    static const uint32_t BUFFER_SIZE = uint32_t(WIDTH) * uint32_t(HEIGHT) / 8; // 5624 bytes
    // panel-native polarity: black frame (0x10): 1 = black; colour frame (0x13): 1 = red
    uint8_t _black_buffer[BUFFER_SIZE];
    uint8_t _red_buffer[BUFFER_SIZE];
    int8_t _temperature_c;
};

#endif
