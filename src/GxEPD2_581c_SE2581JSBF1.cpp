#include "GxEPD2_581c_SE2581JSBF1.h"

GxEPD2_581c_SE2581JSBF1::GxEPD2_581c_SE2581JSBF1(int16_t cs, int16_t dc, int16_t rst, int16_t busy) :
  // busy_timeout is a hang guard, not the expected refresh time: a full refresh takes ~20 s, so give
  // it 2x headroom. At 20 s _waitWhileBusy() times out mid-refresh and reports the panel as done.
  GxEPD2_EPD(cs, dc, rst, busy, LOW, 40000000, WIDTH, HEIGHT, panel, hasColor, hasPartialUpdate, hasFastPartialUpdate)
{
  _black_buffer = (uint8_t*) malloc(BUFFER_SIZE);
  _red_buffer = (uint8_t*) malloc(BUFFER_SIZE);
  if (_black_buffer) memset(_black_buffer, 0x00, BUFFER_SIZE);
  if (_red_buffer) memset(_red_buffer, 0x00, BUFFER_SIZE);
}

void GxEPD2_581c_SE2581JSBF1::clearScreen(uint8_t value) { clearScreen(value, 0xFF); }

void GxEPD2_581c_SE2581JSBF1::clearScreen(uint8_t black_value, uint8_t color_value)
{
  writeScreenBuffer(black_value, color_value);
  refresh(false);
}

void GxEPD2_581c_SE2581JSBF1::writeScreenBuffer(uint8_t value) { writeScreenBuffer(value, 0xFF); }

void GxEPD2_581c_SE2581JSBF1::writeScreenBuffer(uint8_t black_value, uint8_t color_value)
{
  _initial_write = false;
  if (!_black_buffer || !_red_buffer) return; // malloc failed - nothing we can do
  memset(_black_buffer, uint8_t(~black_value), BUFFER_SIZE);
  memset(_red_buffer, uint8_t(~color_value), BUFFER_SIZE);
}

void GxEPD2_581c_SE2581JSBF1::writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, NULL, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_581c_SE2581JSBF1::writeImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (_initial_write) writeScreenBuffer();
  delay(1);
  _writeImageToBuffer(_black_buffer, black, x, y, w, h, invert, mirror_y, pgm);
  _writeImageToBuffer(_red_buffer, color, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_581c_SE2581JSBF1::_writeImageToBuffer(uint8_t* buffer, const uint8_t* bitmap, int16_t x, int16_t y, int16_t w, int16_t h,
                                                bool invert, bool mirror_y, bool pgm)
{
  int16_t wb = (w + 7) / 8;
  x -= x % 8;
  w = wb * 8;
  int16_t x1 = x < 0 ? 0 : x;
  int16_t y1 = y < 0 ? 0 : y;
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x;
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y;
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
        int32_t idx = mirror_y ? j + dx / 8 + int32_t((h - 1 - (i + dy))) * wb : j + dx / 8 + int32_t(i + dy) * wb;
        data = bitmap[idx];
        if (invert) data = ~data;
      }
      buffer[uint32_t(y1 + i) * (WIDTH / 8) + uint32_t(x1 / 8 + j)] = ~data;
    }
  }
}

void GxEPD2_581c_SE2581JSBF1::writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                           int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(bitmap, NULL, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_581c_SE2581JSBF1::writeImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                           int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (_initial_write) writeScreenBuffer();
  delay(1);
  _writeImagePartToBuffer(_black_buffer, black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  _writeImagePartToBuffer(_red_buffer, color, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_581c_SE2581JSBF1::_writeImagePartToBuffer(uint8_t* buffer, const uint8_t* bitmap, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                                    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  int16_t wb_bitmap = (w_bitmap + 7) / 8;
  x_part -= x_part % 8;
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w;
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h;
  x -= x % 8;
  w = 8 * ((w + 7) / 8);
  int16_t x1 = x < 0 ? 0 : x;
  int16_t y1 = y < 0 ? 0 : y;
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x;
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y;
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
        int32_t idx = mirror_y ? x_part / 8 + j + dx / 8 + int32_t((h_bitmap - 1 - (y_part + i + dy))) * wb_bitmap
                               : x_part / 8 + j + dx / 8 + int32_t(y_part + i + dy) * wb_bitmap;
        data = bitmap[idx];
        if (invert) data = ~data;
      }
      buffer[uint32_t(y1 + i) * (WIDTH / 8) + uint32_t(x1 / 8 + j)] = ~data;
    }
  }
}

void GxEPD2_581c_SE2581JSBF1::writeNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(data1, data2, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_581c_SE2581JSBF1::drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(false);
}

void GxEPD2_581c_SE2581JSBF1::drawImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                          int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(false);
}

void GxEPD2_581c_SE2581JSBF1::drawImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(black, color, x, y, w, h, invert, mirror_y, pgm);
  refresh(false);
}

void GxEPD2_581c_SE2581JSBF1::drawImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                          int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(black, color, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(false);
}

void GxEPD2_581c_SE2581JSBF1::drawNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeNative(data1, data2, x, y, w, h, invert, mirror_y, pgm);
  refresh(false);
}

void GxEPD2_581c_SE2581JSBF1::_resetPanel()
{
  if (_rst >= 0)
  {
    digitalWrite(_rst, HIGH); delay(20);
    digitalWrite(_rst, LOW);  delay(10);
    digitalWrite(_rst, HIGH); delay(20);
    digitalWrite(_rst, LOW);  delay(10);
    digitalWrite(_rst, HIGH); delay(10);
  }
  _hibernating = false;
}

void GxEPD2_581c_SE2581JSBF1::_InitDisplay()
{
  _resetPanel();

  // POWER SETTING
  _writeCommand(0x01);
  _writeData(0x07);
  _writeData(0x07); // VGH=20V,VGL=-20V
  _writeData(0x3f); // VDH=15V
  _writeData(0x3f); // VDL=-15V

  // Panel setting - 0x0E is BWR OTP
  _writeCommand(0x00);
  _writeData(0x0E);
  delay(10);

  // Set Resolution
  _writeCommand(0x61);
  _writeData(WIDTH / 256);
  _writeData(WIDTH % 256);
  _writeData(HEIGHT / 256);
  _writeData(HEIGHT % 256);
  
  _writeCommand(0x15);
  _writeData(0x00);
  
  // VCOM AND DATA INTERVAL SETTING
  _writeCommand(0x50);
  _writeData(0x01); // 0x01 instead of 0x11 to avoid inverting colors
  _writeData(0x07);
  
  // TCON SETTING
  _writeCommand(0x60);
  _writeData(0x22);
  
  _init_display_done = true;
}

void GxEPD2_581c_SE2581JSBF1::refresh(bool partial_update_mode)
{
  if (!_black_buffer || !_red_buffer) return; // malloc failed
  _InitDisplay();

  _writeCommand(0x04); // Power on
  _waitWhileBusy("Power on", power_on_time);

  // Send first frame (black) - wait, if it was red when we weren't trying for it, 
  // it might be that 0x10 is RED and 0x13 is BLACK!
  // But let's stick to standard 0x10 = Black, 0x13 = Red for now.
  _writeCommand(0x10);
  _writeData(_black_buffer, BUFFER_SIZE);

  _writeCommand(0x13); // Send second frame (red)
  _writeData(_red_buffer, BUFFER_SIZE);

  _writeCommand(0x12); // display refresh
  _waitWhileBusy("_Update_Full refresh", full_refresh_time);

  powerOff();
  _initial_refresh = false;
}

void GxEPD2_581c_SE2581JSBF1::refresh(int16_t x, int16_t y, int16_t w, int16_t h)
{
  refresh(false);
}

void GxEPD2_581c_SE2581JSBF1::powerOff()
{
  // _writeCommand(0x02); // power off
  // _waitWhileBusy("powerOff", power_off_time);
}

void GxEPD2_581c_SE2581JSBF1::hibernate()
{
  _writeCommand(0x07); // deep sleep
  _writeData(0xA5);
  _hibernating = true;
  _init_display_done = false;
}

void GxEPD2_581c_SE2581JSBF1::setTemperatureC(int8_t degrees_c)
{
  _temperature_c = degrees_c;
}
