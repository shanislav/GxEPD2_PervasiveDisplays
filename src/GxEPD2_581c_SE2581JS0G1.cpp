#include "GxEPD2_581c_SE2581JS0G1.h"
#include <SPI.h>

GxEPD2_581c_SE2581JS0G1::GxEPD2_581c_SE2581JS0G1(int16_t cs, int16_t dc, int16_t rst, int16_t busy,
                                                 int16_t sck, int16_t mosi) :
  // busy_timeout is a hang guard, not the expected refresh time: a full refresh takes ~44 s, so give
  // it 2x headroom. Set to the refresh time it would time out mid-refresh and report the panel done.
  GxEPD2_EPD(cs, dc, rst, busy, LOW, 90000000, WIDTH, HEIGHT, panel, hasColor, hasPartialUpdate,
             hasFastPartialUpdate),
  _otp_read_done(false), _sck_pin(sck), _mosi_pin(mosi), _temperature_c(25)
{
  _black_buffer = (uint8_t*) malloc(PLANE_SIZE);
  _red_buffer = (uint8_t*) malloc(PLANE_SIZE);
  if (_black_buffer) memset(_black_buffer, 0x00, PLANE_SIZE);
  if (_red_buffer) memset(_red_buffer, 0x00, PLANE_SIZE);
}

void GxEPD2_581c_SE2581JS0G1::clearScreen(uint8_t value) { clearScreen(value, 0xFF); }

void GxEPD2_581c_SE2581JS0G1::clearScreen(uint8_t black_value, uint8_t color_value)
{
  writeScreenBuffer(black_value, color_value);
  refresh(false);
}

void GxEPD2_581c_SE2581JS0G1::writeScreenBuffer(uint8_t value) { writeScreenBuffer(value, 0xFF); }

void GxEPD2_581c_SE2581JS0G1::writeScreenBuffer(uint8_t black_value, uint8_t color_value)
{
  _initial_write = false;
  if (!_black_buffer || !_red_buffer) return; // malloc failed - nothing we can do
  // native polarity: 1 = ink. GxEPD2 value 0xFF = white/no-colour -> native 0x00.
  memset(_black_buffer, uint8_t(~black_value), PLANE_SIZE);
  memset(_red_buffer, uint8_t(~color_value), PLANE_SIZE);
}

// Buffer layout is plain row-major in native orientation: byte = y * (WIDTH / 8) + x / 8, MSB is the
// leftmost x. This is exactly PDLS s_getZ/s_getB for small and medium screens.
void GxEPD2_581c_SE2581JS0G1::_writeImageToBuffer(uint8_t* buffer, const uint8_t* bitmap, int16_t x, int16_t y,
                                                  int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (!buffer) return;
  int16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
  x -= x % 8; // byte boundary
  w = wb * 8; // byte boundary
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
      uint8_t data = 0xFF; // white / no colour in GxEPD2 convention
      if (bitmap)
      {
        int32_t idx = mirror_y ? j + dx / 8 + int32_t((h - 1 - (i + dy))) * wb : j + dx / 8 + int32_t(i + dy) * wb;
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
        data = pgm ? pgm_read_byte(&bitmap[idx]) : bitmap[idx];
#else
        data = bitmap[idx];
#endif
        if (invert) data = ~data;
      }
      // invert to panel-native polarity (1 = ink)
      buffer[uint32_t(y1 + i) * (WIDTH / 8) + uint32_t(x1 / 8 + j)] = ~data;
    }
  }
}

void GxEPD2_581c_SE2581JS0G1::_writeImagePartToBuffer(uint8_t* buffer, const uint8_t* bitmap, int16_t x_part, int16_t y_part,
                                                      int16_t w_bitmap, int16_t h_bitmap, int16_t x, int16_t y, int16_t w, int16_t h,
                                                      bool invert, bool mirror_y, bool pgm)
{
  if (!buffer) return;
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
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
        data = pgm ? pgm_read_byte(&bitmap[idx]) : bitmap[idx];
#else
        data = bitmap[idx];
#endif
        if (invert) data = ~data;
      }
      buffer[uint32_t(y1 + i) * (WIDTH / 8) + uint32_t(x1 / 8 + j)] = ~data;
    }
  }
}

void GxEPD2_581c_SE2581JS0G1::writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, NULL, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_581c_SE2581JS0G1::writeImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (_initial_write) writeScreenBuffer();
  _writeImageToBuffer(_black_buffer, black, x, y, w, h, invert, mirror_y, pgm);
  _writeImageToBuffer(_red_buffer, color, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_581c_SE2581JS0G1::writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                             int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(bitmap, NULL, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_581c_SE2581JS0G1::writeImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                             int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (_initial_write) writeScreenBuffer();
  _writeImagePartToBuffer(_black_buffer, black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  _writeImagePartToBuffer(_red_buffer, color, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_581c_SE2581JS0G1::writeNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(data1, data2, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_581c_SE2581JS0G1::drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(false);
}

void GxEPD2_581c_SE2581JS0G1::drawImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                            int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(false);
}

void GxEPD2_581c_SE2581JS0G1::drawImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(black, color, x, y, w, h, invert, mirror_y, pgm);
  refresh(false);
}

void GxEPD2_581c_SE2581JS0G1::drawImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                            int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(black, color, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(false);
}

void GxEPD2_581c_SE2581JS0G1::drawNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeNative(data1, data2, x, y, w, h, invert, mirror_y, pgm);
  refresh(false);
}

// --- panel protocol -------------------------------------------------------------------------

// PDLS b_reset(200, 20, 200, 50, 5) for medium screens
void GxEPD2_581c_SE2581JS0G1::_resetPanel()
{
  delay(200);
  digitalWrite(_rst, HIGH); delay(20);
  digitalWrite(_rst, LOW);  delay(200);
  digitalWrite(_rst, HIGH); delay(50);
  digitalWrite(_cs, HIGH);  delay(5);
  _hibernating = false;
}

void GxEPD2_581c_SE2581JS0G1::_spi3Write(uint8_t value)
{
  pinMode(_sck_pin, OUTPUT);
  pinMode(_mosi_pin, OUTPUT);
  for (uint8_t i = 0; i < 8; i++)
  {
    digitalWrite(_mosi_pin, (value & (1 << (7 - i))) ? HIGH : LOW);
    delayMicroseconds(1);
    digitalWrite(_sck_pin, HIGH); delayMicroseconds(1);
    digitalWrite(_sck_pin, LOW);  delayMicroseconds(1);
  }
}

uint8_t GxEPD2_581c_SE2581JS0G1::_spi3Read()
{
  uint8_t value = 0;
  pinMode(_sck_pin, OUTPUT);
  pinMode(_mosi_pin, INPUT);
  for (uint8_t i = 0; i < 8; i++)
  {
    digitalWrite(_sck_pin, HIGH); delayMicroseconds(1);
    value |= digitalRead(_mosi_pin) << (7 - i);
    digitalWrite(_sck_pin, LOW);  delayMicroseconds(1);
  }
  return value;
}

void GxEPD2_581c_SE2581JS0G1::_readOTP()
{
  if (_otp_read_done) return;
  _resetPanel();

  // IMPORTANT (ESP32-S2): the sketch must NOT have called SPI.begin() before this point. On the S2,
  // once SPI.begin() runs on the native FSPI pins, the bit-bang read-back below returns only 0xFF
  // (SPI.end()/gpio_reset_pin() do not recover it), so the OTP - and thus the whole init - is
  // garbage and the panel never boosts. We read the OTP here on untouched pins, then begin HW SPI.
  SPI.end(); // harmless if SPI was never begun; releases it if a later refresh re-reads OTP
  digitalWrite(_sck_pin, LOW);
  pinMode(_sck_pin, OUTPUT);

  digitalWrite(_dc, LOW);
  digitalWrite(_cs, LOW);
  _spi3Write(0xB9); // DRIVER_B OTP read command
  delay(5);
  digitalWrite(_dc, HIGH);
  (void)_spi3Read(); // dummy
  for (uint16_t i = 0; i < 128; i++) _cog_data[i] = _spi3Read();
  digitalWrite(_cs, HIGH);

  pinMode(_mosi_pin, OUTPUT);
  SPI.begin(_sck_pin, -1, _mosi_pin, _cs); // restore HW SPI on our pins

  // ESP32-S2: passing _cs as SPI's SS makes SPI.begin() route that pin (and, via IOMUX, the DC/RST
  // control pins) into the SPI peripheral function, so the manual digitalWrite() in _sendIndexData()
  // no longer drives CS - the panel then never sees a valid frame and never boosts (blank screen).
  // Force the control pins back to plain GPIO outputs after SPI.begin() so we keep manual control.
  pinMode(_dc, OUTPUT);
  pinMode(_rst, OUTPUT);
  pinMode(_cs, OUTPUT); digitalWrite(_cs, HIGH);

  _otp_read_done = true;
  if (_diag_enabled)
  {
    Serial.print("OTP:");
    for (uint16_t i = 0; i < 128; i++)
    {
      if (i % 16 == 0) Serial.println();
      Serial.print(' '); if (_cog_data[i] < 0x10) Serial.print('0'); Serial.print(_cog_data[i], HEX);
    }
    Serial.println();
  }
}

void GxEPD2_581c_SE2581JS0G1::_sendIndexData(uint8_t reg, const uint8_t* data, uint32_t n)
{
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(_dc, LOW);
  digitalWrite(_cs, LOW);
  SPI.transfer(reg);
  digitalWrite(_dc, HIGH);
  for (uint32_t i = 0; i < n; i++) SPI.transfer(data[i]);
  digitalWrite(_cs, HIGH);
  SPI.endTransaction();
}

void GxEPD2_581c_SE2581JS0G1::_sendIndexFixed(uint8_t reg, uint8_t value, uint32_t n)
{
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(_dc, LOW);
  digitalWrite(_cs, LOW);
  SPI.transfer(reg);
  digitalWrite(_dc, HIGH);
  for (uint32_t i = 0; i < n; i++) SPI.transfer(value);
  digitalWrite(_cs, HIGH);
  SPI.endTransaction();
}

void GxEPD2_581c_SE2581JS0G1::_sendCommandData8(uint8_t cmd, uint8_t d)
{
  _sendIndexData(cmd, &d, 1);
}

void GxEPD2_581c_SE2581JS0G1::_initial()
{
  _resetPanel();
  _sendCommandData8(0x01, _cog_data[0x10]); // DCTL
}

void GxEPD2_581c_SE2581JS0G1::_sendImageData()
{
  _sendIndexData(0x13, &_cog_data[0x15], 6); // DUW
  _sendIndexData(0x90, &_cog_data[0x0c], 4); // DRFW
  _sendIndexData(0x12, &_cog_data[0x12], 3); // RAM_RW
  _sendIndexData(0x10, _black_buffer, PLANE_SIZE); // first frame

  _sendIndexData(0x12, &_cog_data[0x12], 3); // RAM_RW
  _sendIndexData(0x11, _red_buffer, PLANE_SIZE);  // second frame (film J = colour)
}

void GxEPD2_581c_SE2581JS0G1::_update()
{
  _sendCommandData8(0x05, 0x7D);
  delay(200);
  _sendCommandData8(0x05, 0x00);
  delay(10);
  _sendCommandData8(0xD8, _cog_data[0x1c]); // MS_SYNC
  _sendCommandData8(0xD6, _cog_data[0x1d]); // BVSS

  _sendCommandData8(0xA7, 0x10); delay(100);
  _sendCommandData8(0xA7, 0x00); delay(100);

  _sendCommandData8(0x44, 0x00);
  _sendCommandData8(0x45, 0x80);

  _sendCommandData8(0xA7, 0x10); delay(100);
  _sendCommandData8(0xA7, 0x00); delay(100);

  _sendCommandData8(0x44, 0x06);
  uint8_t indexTemperature = uint8_t(_temperature_c) * 2 + 0x50;
  _sendCommandData8(0x45, indexTemperature); // 0x82 @ 25 C

  _sendCommandData8(0xA7, 0x10); delay(100);
  _sendCommandData8(0xA7, 0x00); delay(100);

  _sendCommandData8(0x60, _cog_data[0x0b]); // TCON
  _sendCommandData8(0x61, _cog_data[0x1b]); // STV_DIR
  _sendCommandData8(0x02, _cog_data[0x11]); // VCOM

  // DC/DC soft-start, DRIVER_B table at OTP offset 0x28 (four 8-byte frames)
  const uint8_t offsetFrame = 0x28;
  const uint8_t filter09 = 0xFF;
  for (uint8_t stage = 0; stage < 4; stage++)
  {
    uint8_t offset = offsetFrame + 0x08 * stage;
    uint8_t FORMAT = _cog_data[offset] & 0x80;
    uint8_t REPEAT = _cog_data[offset] & 0x7F;

    if (FORMAT > 0) // Format 1: ramp PHL/PHH
    {
      uint8_t PHL_PHH[2];
      PHL_PHH[0] = _cog_data[offset + 1];
      PHL_PHH[1] = _cog_data[offset + 2];
      uint8_t PHL_VAR = _cog_data[offset + 3];
      uint8_t PHH_VAR = _cog_data[offset + 4];
      uint8_t BST_SW_a = _cog_data[offset + 5] & filter09;
      uint8_t BST_SW_b = _cog_data[offset + 6] & filter09;
      uint8_t DELAY_SCALE = _cog_data[offset + 7] & 0x80;
      uint16_t DELAY_VALUE = _cog_data[offset + 7] & 0x7F;

      for (uint8_t i = 0; i < REPEAT; i++)
      {
        _sendCommandData8(0x09, BST_SW_a);
        PHL_PHH[0] += PHL_VAR;
        PHL_PHH[1] += PHH_VAR;
        _sendIndexData(0x51, PHL_PHH, 2);
        _sendCommandData8(0x09, BST_SW_b);
        if (DELAY_SCALE > 0) delay(DELAY_VALUE);
        else delayMicroseconds(10 * DELAY_VALUE);
      }
    }
    else // Format 2: fixed switches, two delays
    {
      uint8_t BST_SW_a = _cog_data[offset + 1] & filter09;
      uint8_t BST_SW_b = _cog_data[offset + 2] & filter09;
      uint8_t DELAY_a_SCALE = _cog_data[offset + 3] & 0x80;
      uint16_t DELAY_a_VALUE = _cog_data[offset + 3] & 0x7F;
      uint8_t DELAY_b_SCALE = _cog_data[offset + 4] & 0x80;
      uint16_t DELAY_b_VALUE = _cog_data[offset + 4] & 0x7F;

      for (uint8_t i = 0; i < REPEAT; i++)
      {
        _sendCommandData8(0x09, BST_SW_a);
        if (DELAY_a_SCALE > 0) delay(DELAY_a_VALUE);
        else delayMicroseconds(10 * DELAY_a_VALUE);
        _sendCommandData8(0x09, BST_SW_b);
        if (DELAY_b_SCALE > 0) delay(DELAY_b_VALUE);
        else delayMicroseconds(10 * DELAY_b_VALUE);
      }
    }
  }

  // Display refresh start
  _waitWhileBusy("_update start", power_on_time);
  _sendCommandData8(0x15, 0x3C);
}

void GxEPD2_581c_SE2581JS0G1::_powerOff()
{
  _waitWhileBusy("powerOff", full_refresh_time);
  // sequence for eScreen_EPD_581_JS_0B
  _sendCommandData8(0x09, 0x7F);
  _sendCommandData8(0x05, 0x3D);
  _sendCommandData8(0x09, 0x7E);
  delay(15);
  _sendCommandData8(0x09, 0x00);
  _power_is_on = false;
}

void GxEPD2_581c_SE2581JS0G1::refresh(bool partial_update_mode)
{
  if (!_black_buffer || !_red_buffer) return;
  _readOTP();
  _initial();
  _sendImageData();
  _update();
  _waitWhileBusy("refresh", full_refresh_time);
  _power_is_on = true;
  _initial_refresh = false;
}

void GxEPD2_581c_SE2581JS0G1::refresh(int16_t x, int16_t y, int16_t w, int16_t h)
{
  refresh(false);
}

void GxEPD2_581c_SE2581JS0G1::powerOff()
{
  if (_power_is_on) _powerOff();
}

void GxEPD2_581c_SE2581JS0G1::hibernate()
{
  powerOff();
  _hibernating = true;
}

void GxEPD2_581c_SE2581JS0G1::setTemperatureC(int8_t degrees_c)
{
  _temperature_c = degrees_c;
}
