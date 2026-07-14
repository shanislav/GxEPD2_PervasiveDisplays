// Display Library for Pervasive Displays SPI e-paper panels (iTC, film J "Spectra" BWR).
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// Driver for the Pervasive Displays 9.7" BWR panel used in SES-imagotag / VUSION 9.7 GU111 shelf
// labels (model EDG4-0970-A) - panel TE2969JS0B4, 672x960, 3-colour, DUAL-COG (master + slave).
//
// Unlike the 2.66"/5.81" panels this is a "large" dual-controller panel: two COGs each drive half
// of the glass (the 960 dimension is split at 480 - master = 0..479, slave = 480..959). They share
// DC/RST/SDA/SCL and are selected by separate CS lines (M-CS, S-CS); the two FPCs are also wired
// together by cascade lines (FSYNC/LNSYNC/CLK) that sync the halves in hardware.
//
// The command sequence is the Pervasive iTC "0B" large-screen protocol (a faithful port of the
// PDLS COG_LargeCJ path): a 128-byte OTP read (command 0xB9 over 3-wire SPI) supplies the per-panel
// init parameters (DCTL, VCOM, TCON, STV_DIR, MS_SYNC, BVSS, DUW/DRFW addressing and the DC/DC
// soft-start tables). This panel is a genuine Pervasive iTC controller (it answers the OTP read),
// so - unlike the 5.81" VUSION variant - the stock protocol works.
//
// Reference: Pervasive Displays PDLS_EXT3_Basic_Global (COG_LargeCJ_*), screen eScreen_EPD_969_JS_0B.
// GxEPD2 dual/quad-controller structure follows GxEPD2_1248c by Jean-Marc Zingg.
//
// Author: Shano (with Claude), in the style of GxEPD2 by Jean-Marc Zingg
//
// Library: https://github.com/ZinggJM/GxEPD2

#ifndef _GxEPD2_970c_TE2969JS0B4_H_
#define _GxEPD2_970c_TE2969JS0B4_H_

#include <GxEPD2_EPD.h>

class GxEPD2_970c_TE2969JS0B4 : public GxEPD2_EPD
{
  public:
    // attributes
    // Presented to GxEPD2 as landscape 960 x 672. GxEPD2 X = panel "y" (0..959, split at 480 into
    // master/slave), GxEPD2 Y = panel "x" (0..671). Matches the PDLS s_getZ layout so the internal
    // buffer splits cleanly: first half of bytes -> master, second half -> slave.
    static const uint16_t WIDTH = 960;
    static const uint16_t WIDTH_VISIBLE = WIDTH;
    static const uint16_t HEIGHT = 672;
    static const GxEPD2::Panel panel = GxEPD2::GDEY1248Z51; // no PD enum; closest large 3C
    static const bool hasColor = true;
    static const bool hasPartialUpdate = false; // no RAM window addressing exposed
    static const bool hasFastPartialUpdate = false;
    static const uint16_t power_on_time = 200; // ms
    static const uint16_t power_off_time = 300; // ms
    static const uint16_t full_refresh_time = 40000; // ms, large BWR Spectra full refresh
    static const uint16_t partial_refresh_time = 40000; // ms, no partial mode
    // constructor: master CS, slave CS, shared DC/RST, master BUSY, plus SCK/MOSI for the 3-wire OTP read
    GxEPD2_970c_TE2969JS0B4(int16_t cs_master, int16_t cs_slave, int16_t dc, int16_t rst, int16_t busy,
                            int16_t sck, int16_t mosi);
    // methods (virtual)
    void clearScreen(uint8_t value = 0xFF);
    void clearScreen(uint8_t black_value, uint8_t color_value);
    void writeScreenBuffer(uint8_t value = 0xFF);
    void writeScreenBuffer(uint8_t black_value, uint8_t color_value);
    void writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                        int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void writeImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void writeImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                        int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void writeNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void drawImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                       int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void drawImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void drawImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                       int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void drawNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void refresh(bool partial_update_mode = false);
    void refresh(int16_t x, int16_t y, int16_t w, int16_t h);
    void powerOff();
    void hibernate();
    void setTemperatureC(int8_t degrees_c);
  private:
    // pixel-level plane write using the PDLS s_getZ / s_getB layout (transposed + master/slave split)
    void _ensureBuffers(); // lazy PSRAM/heap alloc at setup() time (not in the global constructor)
    void _drawPixelToBuffer(uint8_t* buffer, int16_t X, int16_t Y, bool ink);
    void _writeImageToBuffer(uint8_t* buffer, const uint8_t* bitmap, int16_t x, int16_t y, int16_t w, int16_t h,
                             bool invert, bool mirror_y, bool pgm);
    // panel protocol (port of PDLS COG_LargeCJ, DRIVER_B)
    void _resetPanel();
    void _spi3Write(uint8_t value);
    uint8_t _spi3Read();
    void _readOTP();                                   // 128 bytes via 0xB9 (master only)
    void _select(uint8_t sel);                         // CS lines: 0=master, 1=slave, 2=both
    void _sendIndexData(uint8_t reg, const uint8_t* data, uint32_t n, uint8_t sel);
    void _sendIndexFixed(uint8_t reg, uint8_t value, uint32_t n, uint8_t sel);
    void _sendCommandData8(uint8_t cmd, uint8_t d, uint8_t sel);
    void _initial();                                   // DCTL
    void _sendImageData();                             // DUW/DRFW + master/slave frames
    void _update();                                    // init regs + DC/DC soft-start + refresh
    void _powerOff();
  private:
    // layout matches PDLS: u_bufferSizeV = 672, u_bufferSizeH = 120, u_pageColourSize = 80640
    static const uint32_t PLANE_SIZE = uint32_t(WIDTH) * uint32_t(HEIGHT) / 8; // 80640 bytes/plane
    static const uint32_t SUB_SIZE = PLANE_SIZE / 2;                            // 40320 per half
    // 161 KB total - allocated on the heap (malloc), not as static object members, so it doesn't
    // blow the ESP32-C3 data RAM.
    uint8_t* _black_buffer;
    uint8_t* _red_buffer;
    uint8_t _cog_data[128];
    bool _otp_read_done;
    int16_t _cs_master, _cs_slave, _sck_pin, _mosi_pin;
    int8_t _temperature_c;
    // CS select codes
    static const uint8_t SEL_MASTER = 0;
    static const uint8_t SEL_SLAVE = 1;
    static const uint8_t SEL_BOTH = 2;
};

#endif
