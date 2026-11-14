# KbSwitch – Architecture

## Goal
Standalone box that turns a USB keyboard into a Nintendo Switch controller. No PC needed. Two output modes:
- **Wired** – USB cable to Switch
- **Bluetooth** – wireless, emulates a Pro Controller

```
                                          ┌── micro-USB (native USB device) ──> Switch   [wired]
[Keyboard] ─USB-A─> GP0/GP1 (PIO-USB host) ─ Pico 2 W
                                          └── CYW43 Bluetooth Classic ~~~~~~~~> Switch   [bluetooth]
```

## Hardware
| Part | Use |
|---|---|
| Raspberry Pi Pico 2 W (RP2350 + CYW43439) | Main board |
| USB-A female socket breakout | Keyboard port, driven by PIO-USB |
| 2× 22 Ω resistor (optional) | Series on D+/D- |
| Micro-USB cable + USB-A(f)→USB-C(m) adapter | Pico → Switch (wired) / power bank (BT mode) |
| USB power bank or charger | Power in BT mode |

### Wiring (keyboard socket)
| Socket | Pico |
|---|---|
| VBUS | VBUS (pin 40) |
| D+ | GP2 (pin 4) |
| D- | GP3 (pin 5, must be D+ pin + 1) |
| GND | GND (pin 3) |

GP0/GP1 stay free for the debug UART.

### Debug UART (optional)
- Pico TX = GP0 (pin 1), 115200 8N1, prints mount/unmount logs.
- Uno R3 can be used as a USB-serial adapter: Uno RESET → GND, Pico GP0 → Uno pin 1 (TX), GND ↔ GND. Open the Uno's COM port. Only Pico TX is connected (Uno RX side is 5 V).

- Pico must be powered through micro-USB so VBUS carries 5 V for the keyboard.
- Some power banks shut off at low current draw → use one with "always on"/low-current mode if needed.

## Why Pico 2 W
- PIO → software USB host on 2 GPIOs (second USB port), keyboards are low/full speed → fine.
- Native USB device port → wired controller.
- CYW43439 supports **Bluetooth Classic** (Pro Controller is BT Classic HID, BLE not enough).
- Instant boot, no OS/SD card, cheap.

Rejected: Uno R3 (no USB host, no BT, too slow for bit-banged host), ESP32-S3 (BLE only), ESP32 (no USB), Pi 3 (no USB device mode), Pi 4 (works, but slow boot, SD card, power-hungry), PC + NXBT (needs PC + Linux).

## Software stack
- **C, Pico SDK 2.x**, CMake, board `pico2_w`
- **TinyUSB** (in SDK) – dual role: rhport0 = device (native), rhport1 = host via PIO-USB
- **Pico-PIO-USB** (git submodule, 0.7.3) – PIO USB host driver. Needs sys clock multiple of 12 MHz → run at **120 MHz** (below RP2350 default 150 MHz, no overclock)
- **BTstack** (in SDK, `pico_btstack_classic`) – BT Classic HID device, L2CAP, SDP, pairing/link-key storage in flash
- Reference for Pro Controller protocol: NXBT, joycontrol, dekuNukem Nintendo_Switch_Reverse_Engineering docs

## Firmware design
### Cores
- **Core 1** – PIO-USB host task only (timing critical). Parses HID keyboard reports, writes pressed-key set to shared state.
- **Core 0** – TinyUSB device task, BTstack/CYW43 loop, keymap, output.
- Shared state protected by spinlock (small struct, copy under lock).

### Resource allocation
| Resource | Owner |
|---|---|
| PIO0, lowest free DMA channels | CYW43 driver (initialised first) |
| PIO1 SM0–2, DMA 11 | PIO-USB host |
| USB rhport 0 | TinyUSB device (Switch) |
| USB rhport 1 | TinyUSB host via PIO-USB (keyboard) |

### Modules (`firmware/src/`)
| File | Role |
|---|---|
| `main.c` | Init clocks, CYW43, launch core 1, main loop, LED |
| `tusb_config.h` | TinyUSB dual-role config |
| `kb_host.c` | Core 1: PIO-USB host, HID boot-protocol keyboards (up to 4, hubs ok) → key bitmap. NKRO later |
| `usb_descriptors.c` | Wired USB device/config/HID/string descriptors |
| `keymap.c` | Key set → `pad_state` (buttons, hat, sticks) via active keymap. Ctrl+Shift+1..9,0 hotkey selects keymap |
| `keymap_def.h` | Authoring macros for `firmware/keymaps/*.c` (`KEYMAP`, `BTN`, `DPAD`, `LSTICK`, `RSTICK`, `HALF_TILT`, `RSTICK_FOLLOW`) |
| `keymaps_registry.c.in` | Template; CMake generates the keymap list from `firmware/keymaps/*.c` |
| `settings.c` | Persistent settings (selected keymap) in flash sector at `flash end − 4 sectors`, below BTstack bank + RP2350-E10 sector. Writes via `flash_safe_execute` (pauses core 1 briefly) |
| `pad_state.h` | Neutral controller state struct used by all outputs |
| `out_usb_horipad.c` | Wired output: HORIPAD-compatible HID (VID `0x0F0D` PID `0x0092`), no handshake needed |
| `procon.c` | Pro Controller protocol core: 0x30 input reports, 0x21 subcommand replies (device info, SPI flash reads for calibration/colors, report mode, player lights, …) |
| `out_bt_procon.c` | BT transport for `procon.c`: SDP record, HID control/interrupt L2CAP channels, CoD `0x002508`, name "Pro Controller" |

### Mode selection
- At boot: wait up to ~2 s for USB enumeration (`tud_mounted()`).
  - Mounted → **wired** mode.
  - Not mounted (power bank) → **Bluetooth** mode.
- Hotkey toggles mode at runtime.
- Onboard LED: solid = wired, slow blink = BT connecting, fast blink = pairing, on = BT connected.

### Bluetooth
- First pairing: Switch on *Controllers → Change Grip/Order*, hold pairing hotkey.
- Link key + Switch address stored in flash via BTstack TLV → auto-reconnect later.
- Input report 0x30 sent at fixed rate (~8–15 ms).

### Wired
- Switch setting: *Pro Controller Wired Communication = ON*.
- 8-byte HORIPAD report: buttons (2 B), hat, LX, LY, RX, RY, vendor.

## Keymaps
- One file per game in `firmware/keymaps/`, auto-collected by CMake (re-run on build when files are added). 1..10 files.
- Sorted filename order = slot: `01_default.c` → 1, `02_ssbu.c` → 2, … 10th → 0.
- **Ctrl + Shift + 1..9, 0** (number row, left or right modifiers) switches. Pad goes neutral only once a digit is in the chord, so Ctrl/Shift held before it still reach the game briefly. Selection saved in flash, restored at boot.
- Format:
  ```c
  #include "keymap_def.h"
  KEYMAP("Zelda") {
      LSTICK(W, UP),          // key names = HID_KEY_* without prefix
      BTN(SPACE, A),          // PAD_BTN_* without prefix
      DPAD(ARROW_UP, UP),
      HALF_TILT(CONTROL_LEFT), // left stick only
      RSTICK_FOLLOW(CAPS_LOCK),// while held: left-stick keys drive the right stick at full tilt, left stick neutral
  };
  ```
- A key may appear in several entries (e.g. `BTN(J, A)` + `HALF_TILT(J)`).
- Max 128 entries per keymap; compiled with `-Wall -Werror` so typos fail the build.
- Key names are US HID positions. German layout: `ö` = `SEMICOLON`, `<` (left of Y) = `EUROPE_2` (NUBS), `ä` = `APOSTROPHE`, `ü` = `BRACKET_LEFT`.

## Default keymap (`01_default.c`)
| Key | Controller |
|---|---|
| W A S D | Left stick |
| I J K L | Right stick |
| Arrow keys | D-pad |
| Space / Shift / E / Q | A / B / X / Y |
| R / F | L / ZL |
| U / O | R / ZR |
| Enter / Backspace | + / - |
| C / V | Left / Right stick click |
| H | Home |
| P | Capture |
| Left Ctrl (hold) | Half-tilt left stick (walk) |
| Ctrl + Shift + 1..9, 0 | Select keymap 1..10 (all keymaps) |
| Pause (planned) | Toggle wired ↔ Bluetooth |
| Scroll Lock (hold 3 s) | BT pairing mode (forget old pairing) |

- Opposing directions (A+D) → center.
- Diagonals normalized so magnitude ≈ full tilt.

## SSBU keymap (`02_ssbu.c`, slot 2)
| Key (DE layout) | Controller |
|---|---|
| W A S D | Left stick |
| J | A + half-tilt left stick (tilt attacks; dash attack while running) |
| K | B |
| L (hold) | Smash: WASD drives right stick at full tilt, left stick neutral |
| Space / Caps Lock | X (jump) |
| Shift (L/R) / ö | ZL (shield) |
| < (NUBS, hold) | Half-tilt left stick |
| Backspace | + |

- Smash key releases the left stick so a held run direction + C-stick isn't read as a dash attack.
- Unmapped: Y, L, R, ZR, -, D-pad, stick clicks, Home, Capture.

## Latency
Path: keyboard → PIO-USB host (core 1, busy loop) → spinlock keyset → core 0 busy loop (`keymap_apply`) → HORIPAD report → Switch.
- No sleeps, debounce, queues or logging in the hot path. Firmware adds < 0.1 ms.
- Gamepad IN endpoint `bInterval = 1` (1 ms, minimum at full speed). Report sent on change, resent every 8 ms otherwise.
- Dominant factor = keyboard's own polling interval (1 ms gaming boards, often 8 ms on cheap ones). Use a 1000 Hz keyboard.
- Known stall: keymap switch erases a flash sector synchronously (~45 ms typ, up to ~400 ms), pausing both cores. Accepted, switching doesn't happen mid-game.

## Roadmap
1. **Wired MVP** – PIO-USB keyboard host + HORIPAD output. *Verified on PC (joy.cpl), Switch untested.* Keymap profiles (Ctrl+Shift+digit) implemented, untested on hardware. MVP LED: off = no keyboard, blink = keyboard but no Switch, on = both.
2. **Bluetooth** – Pro Controller over BTstack. Highest risk part, no known finished Pico implementation.
3. **Optional** – wired Pro Controller mode reusing `procon.c` (USB handshake `0x80` cmds).

## Repo layout (planned)
```
firmware/
  CMakeLists.txt
  src/              modules above
  keymaps/          one keymap per game (01_default.c, 02_ssbu.c, ...)
  lib/Pico-PIO-USB/ submodule
docs/               wiring, flashing, pairing notes
```

## Build
Toolchain lives in git-ignored `.tools/` (ARM GCC 14.2, Pico SDK 2.3.1, prebuilt picotool + pioasm). Alternative: VS Code "Raspberry Pi Pico" extension.
```
git submodule update --init
cd firmware
cmake -S . -B build -G Ninja   # picks up ../.tools automatically, else pass -DPICO_SDK_PATH etc.
cmake --build build
```
Output: `firmware/build/kbswitch.uf2`.

## Flashing
Hold BOOTSEL while plugging in → Pico appears as drive → copy `kbswitch.uf2`.

## Open points
- Verify Pico-PIO-USB RP2350 support + 240 MHz stability.
- Verify CYW43 + BTstack can coexist with PIO-USB timing on core 1.
- Pro Controller BT timing quirks (Switch is strict about report rate).
