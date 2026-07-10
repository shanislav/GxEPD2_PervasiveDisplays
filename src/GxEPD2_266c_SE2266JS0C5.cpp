// Display Library for Pervasive Displays SPI e-paper panels (iTC, film J "Spectra" BWR).
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// Driver for Pervasive Displays 2.66" BWR panel SE2266JS0C5 (SE2266JS0C5), 152x296, iTC driver "0C".
// Command sequence based on Pervasive Displays Application Note (small C/J film panels)
// and reference implementation in PDLS_EXT3_Basic_Global (COG_SmallCJ_* branch).
//
// Author: Shano (with Claude), in the style of GxEPD2 by Jean-Marc Zingg
//
// Library: https://github.com/ZinggJM/GxEPD2

#include "GxEPD2_266c_SE2266JS0C5.h"

// PSR bytes for small C/J film panels other than 3.70"/4.17"/4.37"
// (PDLS COG_SmallCJ_getDataOTP: {0xCF, 0x8D} for all small sizes incl. 2.66")
static const uint8_t SE2266JS0C5_PSR[2] = {0xCF, 0x8D};

GxEPD2_266c_SE2266JS0C5::GxEPD2_266c_SE2266JS0C5(int16_t cs, int16_t dc, int16_t rst, int16_t busy) :
  GxEPD2_EPD(cs, dc, rst, busy, LOW, 40000000, WIDTH, HEIGHT, panel, hasColor, hasPartialUpdate, hasFastPartialUpdate),
  _temperature_c(25)
{
  // start with white screen content (panel-native polarity: 0 = white / no red)
  memset(_black_buffer, 0x00, BUFFER_SIZE);
  memset(_red_buffer, 0x00, BUFFER_SIZE);
}

void GxEPD2_266c_SE2266JS0C5::setTemperatureC(int8_t degrees_c)
{
  _temperature_c = degrees_c;
}

void GxEPD2_266c_SE2266JS0C5::clearScreen(uint8_t value)
{
  clearScreen(value, 0xFF);
}

void GxEPD2_266c_SE2266JS0C5::clearScreen(uint8_t black_value, uint8_t color_value)
{
  writeScreenBuffer(black_value, color_value);
  refresh(false);
}

void GxEPD2_266c_SE2266JS0C5::writeScreenBuffer(uint8_t value)
{
  writeScreenBuffer(value, 0xFF);
}

void GxEPD2_266c_SE2266JS0C5::writeScreenBuffer(uint8_t black_value, uint8_t color_value)
{
  _initial_write = false; // initial full screen buffer clean done
  // GxEPD2 convention: black plane 1 = white, colour plane 1 = no colour
  // panel-native polarity is inverted for both planes
  memset(_black_buffer, uint8_t(~black_value), BUFFER_SIZE);
  memset(_red_buffer, uint8_t(~color_value), BUFFER_SIZE);
}

void GxEPD2_266c_SE2266JS0C5::writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, NULL, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_266c_SE2266JS0C5::writeImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (_initial_write) writeScreenBuffer(); // initial full screen buffer clean
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  _writeImageToBuffer(_black_buffer, black, x, y, w, h, invert, mirror_y, pgm);
  _writeImageToBuffer(_red_buffer, color, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_266c_SE2266JS0C5::_writeImageToBuffer(uint8_t* buffer, const uint8_t* bitmap, int16_t x, int16_t y, int16_t w, int16_t h,
                                                bool invert, bool mirror_y, bool pgm)
{
  int16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
  x -= x % 8; // byte boundary
  w = wb * 8; // byte boundary
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  for (int16_t i = 0; i < h1; i++)
  {
    for (int16_t j = 0; j < w1 / 8; j++)
    {
      uint8_t data = 0xFF; // white / no colour in GxEPD2 convention
      if (bitmap)
      {
        // use wb, h of bitmap for index!
        int32_t idx = mirror_y ? j + dx / 8 + int32_t((h - 1 - (i + dy))) * wb : j + dx / 8 + int32_t(i + dy) * wb;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          data = pgm_read_byte(&bitmap[idx]);
#else
          data = bitmap[idx];
#endif
        }
        else
        {
          data = bitmap[idx];
        }
        if (invert) data = ~data;
      }
      // invert to panel-native polarity (1 = black / 1 = red)
      buffer[uint32_t(y1 + i) * (WIDTH / 8) + uint32_t(x1 / 8 + j)] = ~data;
    }
  }
}

void GxEPD2_266c_SE2266JS0C5::writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                           int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(bitmap, NULL, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_266c_SE2266JS0C5::writeImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                           int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (_initial_write) writeScreenBuffer(); // initial full screen buffer clean
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  _writeImagePartToBuffer(_black_buffer, black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  _writeImagePartToBuffer(_red_buffer, color, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_266c_SE2266JS0C5::_writeImagePartToBuffer(uint8_t* buffer, const uint8_t* bitmap, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                                    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  int16_t wb_bitmap = (w_bitmap + 7) / 8; // width bytes, bitmaps are padded
  x_part -= x_part % 8; // byte boundary
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
  x -= x % 8; // byte boundary
  w = 8 * ((w + 7) / 8); // byte boundary, bitmaps are padded
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  for (int16_t i = 0; i < h1; i++)
  {
    for (int16_t j = 0; j < w1 / 8; j++)
    {
      uint8_t data = 0xFF;
      if (bitmap)
      {
        // use wb_bitmap, h_bitmap of bitmap for index!
        int32_t idx = mirror_y ? x_part / 8 + j + dx / 8 + int32_t((h_bitmap - 1 - (y_part + i + dy))) * wb_bitmap
                               : x_part / 8 + j + dx / 8 + int32_t(y_part + i + dy) * wb_bitmap;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          data = pgm_read_byte(&bitmap[idx]);
#else
          data = bitmap[idx];
#endif
        }
        else
        {
          data = bitmap[idx];
        }
        if (invert) data = ~data;
      }
      buffer[uint32_t(y1 + i) * (WIDTH / 8) + uint32_t(x1 / 8 + j)] = ~data;
    }
  }
}

void GxEPD2_266c_SE2266JS0C5::writeNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  // native for 3C is black plane + colour plane, same as writeImage
  writeImage(data1, data2, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_266c_SE2266JS0C5::drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(false);
}

void GxEPD2_266c_SE2266JS0C5::drawImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                          int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(false);
}

void GxEPD2_266c_SE2266JS0C5::drawImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(black, color, x, y, w, h, invert, mirror_y, pgm);
  refresh(false);
}

void GxEPD2_266c_SE2266JS0C5::drawImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                          int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(black, color, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(false);
}

void GxEPD2_266c_SE2266JS0C5::drawNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeNative(data1, data2, x, y, w, h, invert, mirror_y, pgm);
  refresh(false);
}

void GxEPD2_266c_SE2266JS0C5::refresh(bool partial_update_mode)
{
  // full refresh only: init COG, send both frames, update, power off
  _InitDisplay();
  // Application note par. 4. Input image to the EPD
  _writeCommand(0x10); // first frame (black)
  _writeData(_black_buffer, BUFFER_SIZE);
  _writeCommand(0x13); // second frame (red)
  _writeData(_red_buffer, BUFFER_SIZE);
  _Update_Full();
  _PowerOff();
  _initial_refresh = false;
}

void GxEPD2_266c_SE2266JS0C5::refresh(int16_t x, int16_t y, int16_t w, int16_t h)
{
  // no partial mode on this COG, do a full refresh
  refresh(false);
}

void GxEPD2_266c_SE2266JS0C5::powerOff()
{
  _PowerOff();
}

void GxEPD2_266c_SE2266JS0C5::hibernate()
{
  _PowerOff();
  // no deep sleep command documented for this COG (PDLS suspend only drives GPIOs);
  // lowest power is achieved by cutting panel power externally
  _hibernating = true;
  _init_display_done = false;
}

void GxEPD2_266c_SE2266JS0C5::_InitDisplay()
{
  // Application note par. 2. Power on COG driver
  // PDLS b_reset(5, 5, 10, 5, 5): HIGH 5ms, LOW 10ms, HIGH 5ms
  digitalWrite(_rst, HIGH);
  delay(5);
  digitalWrite(_rst, LOW);
  delay(10);
  digitalWrite(_rst, HIGH);
  delay(5);
  digitalWrite(_cs, HIGH);
  delay(5);
  _hibernating = false;
  // Application note par. 3. Set environment temperature and PSR
  _writeCommand(0x00); // soft reset
  _writeData(0x0E);
  delay(5);
  _writeCommand(0xE5); // input temperature: 0x00 = 0 degC, 0x19 = 25 degC
  _writeData(uint8_t(_temperature_c));
  _writeCommand(0xE0); // activate temperature
  _writeData(0x02);
  _writeCommand(0x00); // PSR
  _writeData(SE2266JS0C5_PSR, 2);
  _init_display_done = true;
}

void GxEPD2_266c_SE2266JS0C5::_Update_Full()
{
  // Application note par. 5. Send updating command
  _waitWhileBusy("_Update_Full pre", full_refresh_time);
  _writeCommand(0x04); // power on
  _waitWhileBusy("_Update_Full power on", power_on_time);
  _writeCommand(0x12); // display refresh
  delay(5);
  _waitWhileBusy("_Update_Full refresh", full_refresh_time);
  _power_is_on = true;
}

void GxEPD2_266c_SE2266JS0C5::_PowerOff()
{
  // Application note par. 5. Turn-off DC/DC
  _writeCommand(0x02); // DC/DC off
  _waitWhileBusy("_PowerOff", power_off_time);
  _power_is_on = false;
}
