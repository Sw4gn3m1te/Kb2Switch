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
| `keymap.c` | Key set → `pad_state` (buttons, hat, sticks). Compile-time table |
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

## Default keymap
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
| Left Ctrl (hold) | Half-tilt sticks (walk) |
| Pause | Toggle wired ↔ Bluetooth |
| Scroll Lock (hold 3 s) | BT pairing mode (forget old pairing) |

- Opposing directions (A+D) → center.
- Diagonals normalized so magnitude ≈ full tilt.

## Roadmap
1. **Wired MVP** – PIO-USB keyboard host + HORIPAD output. *Implemented, builds, untested on hardware.* MVP LED: off = no keyboard, blink = keyboard but no Switch, on = both.
2. **Bluetooth** – Pro Controller over BTstack. Highest risk part, no known finished Pico implementation.
3. **Optional** – wired Pro Controller mode reusing `procon.c` (USB handshake `0x80` cmds), keymap profiles.

## Repo layout (planned)
```
firmware/
  CMakeLists.txt
  src/              modules above
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
