// Display Library for Pervasive Displays SPI e-paper panels (iTC, film J "Spectra" BWR).
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// Driver for the Pervasive Displays 5.81" BWR panel used in SES-imagotag / VUSION 5.9 shelf labels,
// panel SE2581JS0G1 - 720x256, 3-colour.
//
// NOTE: this is NOT the same panel as SE2581JSBF1 (see GxEPD2_581c_SE2581JSBF1.h). Same glass size,
// completely different COG:
//   SE2581JSBF1 - ignores the iTC OTP read, speaks a UC8179-family command set.
//   SE2581JS0G1 - answers the iTC OTP read (0xB9, 128 bytes) and its OTP carries the DRIVER_B
//                 layout (DCTL@0x10, TCON@0x0b, STV_DIR@0x1b, VCOM@0x11, DC/DC soft-start frames
//                 at 0x28/0x30/0x38/0x40). Verified on hardware.
// Their part numbers say so too: "JS0G" parses as film J + driver G in Pervasive's scheme, while
// "JSBF1" does not parse at all. PDLS defines no driver G, but the panel drives fine on the plain
// iTC MediumCJ DRIVER_B path - confirmed by running stock PDLS as eScreen_EPD_581_JS_0B.
//
// This driver is a port of the PDLS COG_MediumCJ path (DRIVER_B), i.e. the same protocol as the
// 9.7" dual-COG panel but with a single COG and a flat buffer layout.
//
// Reference: Pervasive Displays PDLS_EXT3_Basic_Global (COG_MediumCJ_*), screen eScreen_EPD_581_JS_0B.
//
// Author: Shano (with Claude), in the style of GxEPD2 by Jean-Marc Zingg
//
// Library: https://github.com/ZinggJM/GxEPD2

#ifndef _GxEPD2_581c_SE2581JS0G1_H_
#define _GxEPD2_581c_SE2581JS0G1_H_

#include <GxEPD2_EPD.h>

class GxEPD2_581c_SE2581JS0G1 : public GxEPD2_EPD
{
  public:
    // attributes
    // Native (portrait) orientation, same convention as the 2.66" driver: WIDTH is the panel's
    // short axis, HEIGHT the long one. Both panels use the same PDLS s_getZ "default" branch:
    // z = long * (WIDTH / 8) + (short >> 3), bit = 7 - (short % 8), which is a plain row-major
    // buffer in these terms. Use setRotation(3) for a 720 x 256 landscape.
    // (Mapping the long axis onto GxEPD2's X instead transposes - i.e. mirrors - the image.)
    static const uint16_t WIDTH = 256;
    static const uint16_t WIDTH_VISIBLE = WIDTH;
    static const uint16_t HEIGHT = 720;
    static const GxEPD2::Panel panel = GxEPD2::GDEQ0583Z31; // no PD enum; closest 5.8" 3C
    static const bool hasColor = true;
    static const bool hasPartialUpdate = false; // no RAM window addressing exposed
    static const bool hasFastPartialUpdate = false;
    static const uint16_t power_on_time = 200; // ms
    static const uint16_t power_off_time = 300; // ms
    static const uint16_t full_refresh_time = 45000; // ms, measured ~44 s under stock PDLS
    static const uint16_t partial_refresh_time = 45000; // ms, no partial mode
    // constructor: SCK/MOSI are needed for the 3-wire OTP read
    GxEPD2_581c_SE2581JS0G1(int16_t cs, int16_t dc, int16_t rst, int16_t busy, int16_t sck, int16_t mosi);
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
    void _writeImageToBuffer(uint8_t* buffer, const uint8_t* bitmap, int16_t x, int16_t y, int16_t w, int16_t h,
                             bool invert, bool mirror_y, bool pgm);
    void _writeImagePartToBuffer(uint8_t* buffer, const uint8_t* bitmap, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                 int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm);
    // panel protocol (port of PDLS COG_MediumCJ, DRIVER_B)
    void _resetPanel();
    void _spi3Write(uint8_t value);
    uint8_t _spi3Read();
    void _readOTP();                    // 128 bytes via 0xB9
    void _sendIndexData(uint8_t reg, const uint8_t* data, uint32_t n);
    void _sendIndexFixed(uint8_t reg, uint8_t value, uint32_t n);
    void _sendCommandData8(uint8_t cmd, uint8_t d);
    void _initial();                    // DCTL
    void _sendImageData();              // DUW/DRFW/RAM_RW + both frames
    void _update();                     // init regs + DC/DC soft-start + refresh
    void _powerOff();
  private:
    static const uint32_t PLANE_SIZE = uint32_t(WIDTH) * uint32_t(HEIGHT) / 8; // 23040 bytes/plane
    // 46 KB total - allocated on the heap so it doesn't blow the ESP32-C3 data RAM.
    uint8_t* _black_buffer;
    uint8_t* _red_buffer;
    uint8_t _cog_data[128];
    bool _otp_read_done;
    int16_t _sck_pin, _mosi_pin;
    int8_t _temperature_c;
};

#endif
