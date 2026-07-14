// Driver for Pervasive Displays / VUSION 9.7" BWR dual-COG panel TE2969JS0B4 (672x960).
// Faithful port of the PDLS COG_LargeCJ (DRIVER_B) sequence into a GxEPD2 dual-CS driver.
// See the header for the reverse-engineering notes.

#include "GxEPD2_970c_TE2969JS0B4.h"
#include <SPI.h>
#if defined(ESP32)
#include "esp_heap_caps.h"
#endif

static const SPISettings _spi3_unused(4000000, MSBFIRST, SPI_MODE0);

// Allocate an ~80 KB plane. Prefer PSRAM (large; keeps internal RAM free for WiFi/JSON when this
// driver runs inside a bigger firmware), fall back to internal RAM when there is no PSRAM.
static uint8_t* alloc_plane(uint32_t n)
{
#if defined(ESP32)
  uint8_t* p = (uint8_t*) heap_caps_malloc(n, MALLOC_CAP_SPIRAM);
  if (p) return p;
#endif
  return (uint8_t*) malloc(n);
}

GxEPD2_970c_TE2969JS0B4::GxEPD2_970c_TE2969JS0B4(int16_t cs_master, int16_t cs_slave, int16_t dc, int16_t rst,
                                                 int16_t busy, int16_t sck, int16_t mosi) :
  // busy_timeout is a hang guard, not the expected refresh time: a full refresh takes ~40 s, so give
  // it 2x headroom. At 40 s _waitWhileBusy() times out mid-refresh and reports the panel as done.
  GxEPD2_EPD(cs_master, dc, rst, busy, LOW, 90000000, WIDTH, HEIGHT, panel, hasColor, hasPartialUpdate,
             hasFastPartialUpdate),
  _otp_read_done(false), _cs_master(cs_master), _cs_slave(cs_slave), _sck_pin(sck), _mosi_pin(mosi),
  _temperature_c(25)
{
  // Do NOT allocate here. This is typically a global object, so the constructor runs before PSRAM is
  // initialised, and heap_caps_malloc(MALLOC_CAP_SPIRAM) would fail and fall back to internal RAM.
  // Allocate lazily at setup() time (first use) instead - by then PSRAM is up.
  _black_buffer = nullptr;
  _red_buffer = nullptr;
  memset(_cog_data, 0x00, sizeof(_cog_data));
}

void GxEPD2_970c_TE2969JS0B4::_ensureBuffers()
{
  if (_black_buffer && _red_buffer) return;
  if (!_black_buffer) _black_buffer = alloc_plane(PLANE_SIZE);
  if (!_red_buffer) _red_buffer = alloc_plane(PLANE_SIZE);
  if (_black_buffer) memset(_black_buffer, 0x00, PLANE_SIZE);
  if (_red_buffer) memset(_red_buffer, 0x00, PLANE_SIZE);
}

void GxEPD2_970c_TE2969JS0B4::setTemperatureC(int8_t degrees_c) { _temperature_c = degrees_c; }

///////////////////////////////////////////////
// Buffer layout (PDLS s_getZ): X = panel y (0..959, split at 480), Y = panel x (0..671)
//   half offset = (X >= 480) ? SUB_SIZE : 0 ; z = half + Y * (WIDTH/8/2) + (X%480)/8 ; bit = 7-(X%8)
///////////////////////////////////////////////

void GxEPD2_970c_TE2969JS0B4::_drawPixelToBuffer(uint8_t* buffer, int16_t X, int16_t Y, bool ink)
{
  if (!buffer) return; // malloc failed - don't crash
  if ((X < 0) || (X >= int16_t(WIDTH)) || (Y < 0) || (Y >= int16_t(HEIGHT))) return;
  uint16_t x = uint16_t(X);
  uint32_t z = 0;
  if (x >= (WIDTH / 2)) { x -= (WIDTH / 2); z += SUB_SIZE; }
  z += uint32_t(Y) * (WIDTH / 8 / 2) + (x >> 3); // WIDTH/8/2 = 60 bytes per row per half
  uint8_t b = 7 - (uint16_t(X) % 8);
  if (ink) buffer[z] |= (1 << b);
  else buffer[z] &= ~(1 << b);
}

void GxEPD2_970c_TE2969JS0B4::_writeImageToBuffer(uint8_t* buffer, const uint8_t* bitmap, int16_t x, int16_t y,
                                                  int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (!bitmap) return;
  int16_t wb = (w + 7) / 8; // bitmap row bytes (padded)
  for (int16_t j = 0; j < h; j++)
  {
    for (int16_t i = 0; i < w; i++)
    {
      int16_t iy = mirror_y ? (h - 1 - j) : j;
      uint32_t idx = uint32_t(iy) * wb + (i >> 3);
      uint8_t byteval;
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
      byteval = pgm ? pgm_read_byte(&bitmap[idx]) : bitmap[idx];
#else
      byteval = bitmap[idx];
#endif
      bool bit = byteval & (0x80 >> (i & 7)); // 1 = set
      if (invert) bit = !bit;
      // GxEPD2 convention: in the "black" bitmap 0 = black ink; in "color" bitmap 0 = colour ink.
      // Our buffers are panel-native (1 = ink), so ink = !bit.
      _drawPixelToBuffer(buffer, x + i, y + j, !bit);
    }
  }
}

///////////////////////////////////////////////
// GxEPD2 buffer API
///////////////////////////////////////////////

void GxEPD2_970c_TE2969JS0B4::clearScreen(uint8_t value) { clearScreen(value, 0xFF); }
void GxEPD2_970c_TE2969JS0B4::clearScreen(uint8_t black_value, uint8_t color_value)
{
  writeScreenBuffer(black_value, color_value);
  refresh(false);
}
void GxEPD2_970c_TE2969JS0B4::writeScreenBuffer(uint8_t value) { writeScreenBuffer(value, 0xFF); }
void GxEPD2_970c_TE2969JS0B4::writeScreenBuffer(uint8_t black_value, uint8_t color_value)
{
  _ensureBuffers();
  _initial_write = false;
  if (!_black_buffer || !_red_buffer) return; // malloc failed - nothing we can do
  // native polarity: 1 = ink. GxEPD2 value 0xFF = white/no-colour -> native 0x00.
  memset(_black_buffer, uint8_t(~black_value), PLANE_SIZE);
  memset(_red_buffer, uint8_t(~color_value), PLANE_SIZE);
}

void GxEPD2_970c_TE2969JS0B4::writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, NULL, x, y, w, h, invert, mirror_y, pgm);
}
void GxEPD2_970c_TE2969JS0B4::writeImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _ensureBuffers();
  if (_initial_write) writeScreenBuffer();
  delay(1);
  if (black) _writeImageToBuffer(_black_buffer, black, x, y, w, h, invert, mirror_y, pgm);
  if (color) _writeImageToBuffer(_red_buffer, color, x, y, w, h, invert, mirror_y, pgm);
}
void GxEPD2_970c_TE2969JS0B4::writeImagePart(const uint8_t bitmap[], int16_t, int16_t, int16_t, int16_t,
                                             int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{ writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm); }
void GxEPD2_970c_TE2969JS0B4::writeImagePart(const uint8_t* black, const uint8_t* color, int16_t, int16_t, int16_t, int16_t,
                                             int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{ writeImage(black, color, x, y, w, h, invert, mirror_y, pgm); }
void GxEPD2_970c_TE2969JS0B4::writeNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{ writeImage(data1, data2, x, y, w, h, invert, mirror_y, pgm); }

void GxEPD2_970c_TE2969JS0B4::drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{ writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm); refresh(false); }
void GxEPD2_970c_TE2969JS0B4::drawImagePart(const uint8_t bitmap[], int16_t xp, int16_t yp, int16_t wb, int16_t hb, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{ writeImagePart(bitmap, xp, yp, wb, hb, x, y, w, h, invert, mirror_y, pgm); refresh(false); }
void GxEPD2_970c_TE2969JS0B4::drawImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{ writeImage(black, color, x, y, w, h, invert, mirror_y, pgm); refresh(false); }
void GxEPD2_970c_TE2969JS0B4::drawImagePart(const uint8_t* black, const uint8_t* color, int16_t xp, int16_t yp, int16_t wb, int16_t hb, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{ writeImagePart(black, color, xp, yp, wb, hb, x, y, w, h, invert, mirror_y, pgm); refresh(false); }
void GxEPD2_970c_TE2969JS0B4::drawNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{ writeNative(data1, data2, x, y, w, h, invert, mirror_y, pgm); refresh(false); }

///////////////////////////////////////////////
// SPI / CS helpers (shared bus, per-COG chip select)
///////////////////////////////////////////////

void GxEPD2_970c_TE2969JS0B4::_select(uint8_t sel)
{
  digitalWrite(_cs_master, (sel == SEL_MASTER || sel == SEL_BOTH) ? LOW : HIGH);
  digitalWrite(_cs_slave,  (sel == SEL_SLAVE  || sel == SEL_BOTH) ? LOW : HIGH);
}

void GxEPD2_970c_TE2969JS0B4::_sendIndexData(uint8_t reg, const uint8_t* data, uint32_t n, uint8_t sel)
{
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(_dc, LOW);
  _select(sel);
  SPI.transfer(reg);
  digitalWrite(_dc, HIGH);
  for (uint32_t i = 0; i < n; i++) SPI.transfer(data[i]);
  digitalWrite(_cs_master, HIGH);
  digitalWrite(_cs_slave, HIGH);
  SPI.endTransaction();
}

void GxEPD2_970c_TE2969JS0B4::_sendIndexFixed(uint8_t reg, uint8_t value, uint32_t n, uint8_t sel)
{
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(_dc, LOW);
  _select(sel);
  SPI.transfer(reg);
  digitalWrite(_dc, HIGH);
  for (uint32_t i = 0; i < n; i++) SPI.transfer(value);
  digitalWrite(_cs_master, HIGH);
  digitalWrite(_cs_slave, HIGH);
  SPI.endTransaction();
}

void GxEPD2_970c_TE2969JS0B4::_sendCommandData8(uint8_t cmd, uint8_t d, uint8_t sel)
{
  uint8_t v = d;
  _sendIndexData(cmd, &v, 1, sel);
}

///////////////////////////////////////////////
// 3-wire SPI (bit-bang) for the OTP read
///////////////////////////////////////////////

void GxEPD2_970c_TE2969JS0B4::_spi3Write(uint8_t value)
{
  pinMode(_sck_pin, OUTPUT);
  pinMode(_mosi_pin, OUTPUT);
  for (uint8_t i = 0; i < 8; i++)
  {
    digitalWrite(_mosi_pin, (value & (1 << (7 - i))) ? HIGH : LOW);
    delayMicroseconds(1);
    digitalWrite(_sck_pin, HIGH);
    delayMicroseconds(1);
    digitalWrite(_sck_pin, LOW);
    delayMicroseconds(1);
  }
}

uint8_t GxEPD2_970c_TE2969JS0B4::_spi3Read()
{
  uint8_t value = 0;
  pinMode(_sck_pin, OUTPUT);
  pinMode(_mosi_pin, INPUT);
  for (uint8_t i = 0; i < 8; i++)
  {
    digitalWrite(_sck_pin, HIGH);
    delayMicroseconds(1);
    value |= digitalRead(_mosi_pin) << (7 - i);
    digitalWrite(_sck_pin, LOW);
    delayMicroseconds(1);
  }
  return value;
}

void GxEPD2_970c_TE2969JS0B4::_resetPanel()
{
  // PDLS b_reset(200, 20, 200, 200, 5) for large panels
  delay(200);
  digitalWrite(_rst, HIGH);
  delay(20);
  digitalWrite(_rst, LOW);
  delay(200);
  digitalWrite(_rst, HIGH);
  delay(200);
  digitalWrite(_cs_master, HIGH);
  digitalWrite(_cs_slave, HIGH);
  delay(5);
  _hibernating = false;
}

void GxEPD2_970c_TE2969JS0B4::_readOTP()
{
  if (_otp_read_done) return;
  _resetPanel();
  digitalWrite(_cs_slave, HIGH); // OTP read from master only

  // IMPORTANT (ESP32-S2): the sketch must NOT have called SPI.begin() before this point. On the S2,
  // once SPI.begin() runs on the native FSPI pins, the bit-bang read-back below returns only 0xFF
  // (SPI.end()/gpio_reset_pin() do not recover it), so the OTP - and thus the whole init - is
  // garbage and the panel never boosts. We read the OTP here on untouched pins, then begin HW SPI.
  SPI.end(); // harmless if SPI was never begun; releases it if a later refresh re-reads OTP
  digitalWrite(_sck_pin, LOW);
  pinMode(_sck_pin, OUTPUT);

  digitalWrite(_dc, LOW);
  digitalWrite(_cs_master, LOW);
  _spi3Write(0xB9); // DRIVER_B OTP read command
  delay(5);
  digitalWrite(_dc, HIGH);
  (void)_spi3Read(); // dummy
  for (uint16_t i = 0; i < 128; i++) _cog_data[i] = _spi3Read();
  digitalWrite(_cs_master, HIGH);

  pinMode(_mosi_pin, OUTPUT);
  SPI.begin(_sck_pin, -1, _mosi_pin, _cs_master); // restore HW SPI on our pins

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

///////////////////////////////////////////////
// COG_LargeCJ sequence (DRIVER_B)
///////////////////////////////////////////////

void GxEPD2_970c_TE2969JS0B4::_initial()
{
  _resetPanel();
  _sendCommandData8(0x01, _cog_data[0x10], SEL_BOTH); // DCTL
}

void GxEPD2_970c_TE2969JS0B4::_sendImageData()
{
  _sendIndexData(0x13, &_cog_data[0x15], 6, SEL_BOTH); // DUW
  _sendIndexData(0x90, &_cog_data[0x0c], 4, SEL_BOTH); // DRFW

  // Master half: _black_buffer[0..SUB], _red_buffer[0..SUB]
  _sendIndexData(0x12, &_cog_data[0x12], 3, SEL_MASTER); // RAM_RW
  _sendIndexData(0x10, _black_buffer, SUB_SIZE, SEL_MASTER);
  _sendIndexData(0x12, &_cog_data[0x12], 3, SEL_MASTER);
  _sendIndexData(0x11, _red_buffer, SUB_SIZE, SEL_MASTER);

  // Slave half: _black_buffer[SUB..2SUB], _red_buffer[SUB..2SUB]
  _sendIndexData(0x12, &_cog_data[0x12], 3, SEL_SLAVE);
  _sendIndexData(0x10, _black_buffer + SUB_SIZE, SUB_SIZE, SEL_SLAVE);
  _sendIndexData(0x12, &_cog_data[0x12], 3, SEL_SLAVE);
  _sendIndexData(0x11, _red_buffer + SUB_SIZE, SUB_SIZE, SEL_SLAVE);
}

void GxEPD2_970c_TE2969JS0B4::_update()
{
  _sendCommandData8(0x05, 0x7D, SEL_BOTH); delay(200);
  _sendCommandData8(0x05, 0x00, SEL_BOTH); delay(10);
  _sendCommandData8(0xD8, _cog_data[0x1c], SEL_BOTH); // MS_SYNC
  _sendCommandData8(0xD6, _cog_data[0x1d], SEL_BOTH); // BVSS
  _sendCommandData8(0xA7, 0x10, SEL_BOTH); delay(100);
  _sendCommandData8(0xA7, 0x00, SEL_BOTH); delay(100);

  const uint8_t indexTemperature = uint8_t(_temperature_c) * 2 + 0x50; // 0x82 @ 25C
  for (uint8_t half = 0; half < 2; half++)
  {
    uint8_t sel = (half == 0) ? SEL_MASTER : SEL_SLAVE;
    _sendCommandData8(0x44, 0x00, sel);
    _sendCommandData8(0x45, 0x80, sel);
    _sendCommandData8(0xA7, 0x10, sel); delay(100);
    _sendCommandData8(0xA7, 0x00, sel); delay(100);
    _sendCommandData8(0x44, 0x06, sel);
    _sendCommandData8(0x45, indexTemperature, sel);
    _sendCommandData8(0xA7, 0x10, sel); delay(100);
    _sendCommandData8(0xA7, 0x00, sel); delay(100);
  }

  _sendCommandData8(0x60, _cog_data[0x0b], SEL_BOTH);   // TCON
  _sendCommandData8(0x61, _cog_data[0x1b], SEL_MASTER); // STV_DIR (master only)
  _sendCommandData8(0x02, _cog_data[0x11], SEL_BOTH);   // VCOM

  // DC/DC soft-start from OTP tables (DRIVER_B offset 0x28, 4 stages)
  const uint8_t offsetFrame = 0x28;
  for (uint8_t stage = 0; stage < 4; stage++)
  {
    uint8_t offset = offsetFrame + 0x08 * stage;
    uint8_t FORMAT = _cog_data[offset] & 0x80;
    uint8_t REPEAT = _cog_data[offset] & 0x7f;
    if (FORMAT > 0) // format 1
    {
      uint8_t PHL_PHH[2] = { _cog_data[offset + 1], _cog_data[offset + 2] };
      uint8_t PHL_VAR = _cog_data[offset + 3], PHH_VAR = _cog_data[offset + 4];
      uint8_t BST_SW_a = _cog_data[offset + 5], BST_SW_b = _cog_data[offset + 6];
      uint8_t DELAY_SCALE = _cog_data[offset + 7] & 0x80;
      uint16_t DELAY_VALUE = _cog_data[offset + 7] & 0x7f;
      for (uint8_t i = 0; i < REPEAT; i++)
      {
        _sendCommandData8(0x09, BST_SW_a, SEL_BOTH);
        PHL_PHH[0] += PHL_VAR; PHL_PHH[1] += PHH_VAR;
        _sendIndexData(0x51, PHL_PHH, 2, SEL_BOTH);
        _sendCommandData8(0x09, BST_SW_b, SEL_BOTH);
        if (DELAY_SCALE > 0) delay(DELAY_VALUE); else delayMicroseconds(10 * DELAY_VALUE);
      }
    }
    else // format 2
    {
      uint8_t BST_SW_a = _cog_data[offset + 1], BST_SW_b = _cog_data[offset + 2];
      uint8_t DA_SCALE = _cog_data[offset + 3] & 0x80; uint16_t DA_VALUE = _cog_data[offset + 3] & 0x7f;
      uint8_t DB_SCALE = _cog_data[offset + 4] & 0x80; uint16_t DB_VALUE = _cog_data[offset + 4] & 0x7f;
      for (uint8_t i = 0; i < REPEAT; i++)
      {
        _sendCommandData8(0x09, BST_SW_a, SEL_BOTH);
        if (DA_SCALE > 0) delay(DA_VALUE); else delayMicroseconds(10 * DA_VALUE);
        _sendCommandData8(0x09, BST_SW_b, SEL_BOTH);
        if (DB_SCALE > 0) delay(DB_VALUE); else delayMicroseconds(10 * DB_VALUE);
      }
    }
  }

  _waitWhileBusy("refresh", full_refresh_time);
  _sendCommandData8(0x15, 0x3C, SEL_BOTH); // display refresh start
}

void GxEPD2_970c_TE2969JS0B4::_powerOff()
{
  _waitWhileBusy("powerOff", full_refresh_time);
  // sequence for eScreen_EPD_969_JS_0B
  _sendCommandData8(0x09, 0x7F, SEL_BOTH);
  _sendCommandData8(0x05, 0x3D, SEL_BOTH);
  _sendCommandData8(0x09, 0x7E, SEL_BOTH);
  delay(15);
  _sendCommandData8(0x09, 0x00, SEL_BOTH);
  _power_is_on = false;
}

///////////////////////////////////////////////
// public refresh / power
///////////////////////////////////////////////

void GxEPD2_970c_TE2969JS0B4::refresh(bool)
{
  _ensureBuffers();
  if (!_black_buffer || !_red_buffer) return; // malloc failed
  // make sure the extra pins (slave CS, sck, mosi) are outputs
  pinMode(_cs_master, OUTPUT);
  pinMode(_cs_slave, OUTPUT);
  digitalWrite(_cs_master, HIGH);
  digitalWrite(_cs_slave, HIGH);

  _readOTP();       // once; includes a reset
  _initial();       // reset + DCTL
  _sendImageData(); // DUW/DRFW + master/slave frames
  _update();        // init regs + DC/DC soft-start + refresh start
  _powerOff();
  _initial_refresh = false;
}

void GxEPD2_970c_TE2969JS0B4::refresh(int16_t, int16_t, int16_t, int16_t) { refresh(false); }

void GxEPD2_970c_TE2969JS0B4::powerOff() { /* already powered off after refresh */ }

void GxEPD2_970c_TE2969JS0B4::hibernate()
{
  _hibernating = true;
  _otp_read_done = false; // re-read OTP after a cold start
}
