# GxEPD2_PervasiveDisplays

Add-on driver classes for **[GxEPD2](https://github.com/ZinggJM/GxEPD2)** that add support for
**Pervasive Displays iTC "Spectra" BWR** e-paper panels — including the OEM panels found in
**SES-imagotag / VUSION electronic shelf labels (ESLs)** — which stock GxEPD2 does not support.

This is a **companion library**: you install it *alongside* GxEPD2, you don't replace GxEPD2.
The driver classes derive from `GxEPD2_EPD` and are used with the standard `GxEPD2_3C` template,
so all of GxEPD2 / Adafruit_GFX drawing works exactly as usual.

> These are the first known GxEPD2 drivers for Pervasive Displays / VUSION panels. The 5.81"
> VUSION panel in particular was fully reverse-engineered (no public datasheet or driver exists).

![VUSION 5.9 panel driven by an ESP32-C3 running this library](docs/demo.jpg)

---

## Supported panels

| Class | Panel | Size | Res | Colour | Notes |
|-------|-------|------|-----|--------|-------|
| `GxEPD2_266c_E2266JS0C` | Pervasive Displays **E2266JS0C** (`SE2266JS0C5`) — found in **SES-imagotag / VUSION 2.6 BWR GU110**, model EDG3-0260-A | 2.66" | 296×152 | BWR | standard iTC "small" COG |
| `GxEPD2_581c_2581JSBF1` | **SES-imagotag / VUSION 5.9 BWR GU110** (`2581JSBF1`, model EDG3-0590-A) | 5.81" | 720×256 | BWR | GD7965/UC8179, reverse-engineered |

Both panels come from recycled **VUSION electronic shelf labels**. The 2.66" is a standard catalogued
Pervasive part (works with the normal iTC protocol); the 5.81" is an OEM variant that had to be
reverse-engineered (see notes below).

Both are **3-colour** (black / white / red). Full refresh only (no partial update).

---

## Installation

### Arduino IDE
1. Install **GxEPD2** and **Adafruit GFX Library** via Library Manager.
2. Download this repo as ZIP → *Sketch → Include Library → Add .ZIP Library*.
3. Open an example: *File → Examples → GxEPD2_PervasiveDisplays*.

### PlatformIO
```ini
lib_deps =
    zinggjm/GxEPD2@^1.6.6
    adafruit/Adafruit GFX Library
    adafruit/Adafruit BusIO
    https://github.com/shanislav/GxEPD2_PervasiveDisplays.git
```

---

## Wiring (example: ESP32-C3 Super Mini)

| Panel pin | ESP32-C3 GPIO |
|-----------|---------------|
| CS   | 10 |
| DC   | 5  |
| RST  | 3  |
| BUSY | 1  |
| SCLK | 6  |
| MOSI / SDA | 7 |
| Panel power ON/OFF | 4 → drives the tag's **on-board** power MOSFET (LOW = on). For battery / power-gating — see [Low power](#low-power-battery) |

(The panel is write-only, so SPI MISO is unused — pass `-1` to `SPI.begin`.)

The panels need **3.3 V** on both supply *and* data lines. A boost/level circuit is usually
already on the ESL's flex-to-breakout adapter.

### Where to solder on the VUSION 5.9 board

If you're recycling a **VUSION 5.9 (2581JSBF1)** shelf label, these are the test points on the
original PCB (desolder its MCU first). The board **already has the power MOSFET on it**, so
`ON/OFF` is its enable line — wire it **straight to an ESP GPIO** (we use GPIO4); no external
MOSFET needed. Driving it LOW powers the panel, HIGH cuts it (see [Low power](#low-power-battery)).
The rest of the test points go to the ESP32-C3 pins in the table above.

![VUSION 5.9 test points](docs/wiring_testpoints_vusion59.jpg)

…and the same points with wires soldered on:

![VUSION 5.9 soldered wiring](docs/wiring_soldered_vusion59.jpg)

Board for reference — [front (MCU side)](docs/board_vusion59_front.jpg) ·
[back (bare)](docs/board_vusion59.jpg).

### ⚠️ Do NOT wire panel signals to the ESP32-C3 strapping pins

The ESP32-C3 has **strapping pins** (notably **GPIO9 = BOOT** and **GPIO2**) that are sampled at
power-on to decide the boot mode. **Do not route DC / BUSY / any panel signal through them** unless
you really know what you're doing. The pinout above deliberately avoids them (DC on 5, BUSY on 1).

Why it bites you (learned the hard way):

- **GPIO9 = BOOT.** If it's LOW at power-on the chip enters *download mode* and your firmware never
  runs. If you put **DC on GPIO9** and the panel can be **unpowered** (battery / power-gated via a
  MOSFET), an unpowered panel drags GPIO9 LOW on cold boot → the board silently fails to start.
  The classic symptom: **cold power-up does nothing, but pressing RST "fixes" it** (a reset button
  doesn't re-sample the straps). → Keep DC on a normal pin (we use **GPIO5**).
- **GPIO2** must not be held LOW at boot either — a panel BUSY on GPIO2 can block boot/flash.
  → BUSY on **GPIO1**.

If you can't flash with the panel connected, disconnect the offending signal, flash, reconnect —
or just move it to a non-strapping pin.

---

## Usage

```cpp
#include <SPI.h>
#include <GxEPD2_3C.h>
#include "GxEPD2_581c_2581JSBF1.h"   // or GxEPD2_266c_E2266JS0C.h

GxEPD2_3C<GxEPD2_581c_2581JSBF1, GxEPD2_581c_2581JSBF1::HEIGHT> display(
  GxEPD2_581c_2581JSBF1(/*CS=*/10, /*DC=*/5, /*RST=*/3, /*BUSY=*/1)); // DC on 5, NOT 9 (strapping)

void setup() {
  SPI.begin(/*SCLK=*/6, /*MISO=*/-1, /*MOSI=*/7, /*CS=*/10); // MISO unused (write-only panel)
  display.init(115200, true, 2, false);
  display.setRotation(0);

  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_RED);
    display.setCursor(20, 40);
    display.print("Hello e-paper!");
  } while (display.nextPage());
}

void loop() {}
```

A full refresh takes ~20 s. See `examples/` for both panels.

---

## Panel notes & limitations

### E2266JS0C (2.66")
- iTC "small C/J" COG. PSR value `{0xCF, 0x8D}` is hardcoded (not read from OTP).
- Data as two 1bpp planes: black (`0x10`) + red (`0x13`), 5624 bytes each.

### 2581JSBF1 (VUSION 5.9") — reverse-engineered
This is an **OEM panel with no public datasheet**. It shares the size of the public Pervasive
`E2581JS0B` but uses a **different controller (GD7965 / UC8179)**, so Pervasive's own driver does
**not** work on it. Key findings (also documented in the driver header):

- **Addressed as 720×512, not 720×256.** The controller RAM spans 720×512 but only the **top 256
  rows** are wired to the glass. `HEIGHT` is therefore **512**; each plane is 46080 bytes. Sending
  only half left half the panel undriven. **Draw in Y = 0…255**; anything below is stored in RAM
  but invisible.
- **Panel setting (`0x00`) = `0x0E`.** The usual 3C value `0x0F` *hangs* this controller.
- **VCOM/data-interval (`0x50`) = `0x01, 0x07`.** `0x01` (not `0x11`) avoids inverted colours.
- **No power-off.** Sending `0x02` (power off) cuts the DC/DC before the red pigment sets, making
  the fresh image fade. The driver deliberately skips it; the bistable panel holds its image.

**Origin:** SES-imagotag / VUSION 5.9 BWR GU110 (model EDG3-0590-A) recycled shelf label. The tag's
original MCU is a Silicon Labs EFR32FG22 (secure-locked), so the panel's original waveform/params
were not recoverable — everything here was found empirically on hardware.

### Memory (ESP32-C3)
The 720×512 3C buffer is ~92 KB. If you also run WiFi/WebServer you can run out of RAM. Use paged
rendering — with GxEPD2 pass a smaller page height as the template's second argument so the library
draws in horizontal bands.

### Low power (battery)
These panels are **bistable** — they hold the image with **zero current**. But the driver
deliberately skips the COG's soft power-off (`0x02`, which makes the image fade on this panel), so
the COG's DC/DC stays on and keeps drawing current. For battery use, **cut the whole panel supply**.
On the VUSION tag board the power MOSFET is already there — just drive its `ON/OFF` line from a GPIO.
On a bare panel breakout, add your own high-side MOSFET / load switch. Either way:

```
power ON  ->  init + draw + refresh  ->  power OFF  ->  deep sleep
                                          (image stays on screen at 0 mA)
```

Cut power **after** the refresh finishes — the pigment is already set, so the image holds. The
`HelloWorld_2581JSBF1` example does exactly this (power-gate + deep sleep). Gotcha: park the
SPI/DC/RST lines LOW before cutting power, so they don't back-power the unpowered panel through
its ESD diodes.

### Driving images from a server (e.g. cropping to 256 px)
Because the panel reports/addresses 720×512 but only shows the top 256 rows, generate/serve your
image at **720×256** and draw it into the top of the buffer.

---

## Credits & licence

- Built on **[GxEPD2](https://github.com/ZinggJM/GxEPD2)** by **Jean-Marc Zingg** — the driver
  classes follow its structure and use its `GxEPD2_EPD` base and `GxEPD2_3C` template.
- Reverse-engineering reference: Pervasive Displays [PDLS](https://github.com/PervasiveDisplays)
  driver sources; the ESL-hacking community ([OpenEPaperLink](https://github.com/OpenEPaperLink),
  [atc1441](https://github.com/atc1441)).
- Panel reverse-engineering & drivers: Shano, 2026.

Licensed under **GPL-3.0** (same as GxEPD2), see [LICENSE](LICENSE). You may use, modify and
redistribute freely under the same licence; keep attribution.

## Contributing
Have another Pervasive Displays / VUSION / ESL panel? OTP dumps, waveform notes and new driver
classes are very welcome — open an issue or PR.
