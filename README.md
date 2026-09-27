# ESP32 Dedicated Portable Music Player

High-resolution portable offline audio player firmware designed for the ESP32-WROVER-E (DevKitC) paired with a 2.9-inch 296x128 monochrome SPI E-Paper display.

---

## Hardware Specifications

| Component | Specification |
| :--- | :--- |
| **MCU** | ESP32-WROVER-E (Dual-core Xtensa LX6 @ 240 MHz, 8 MB PSRAM, 4 MB Flash) |
| **Display** | WeAct Studio 2.9" Monochrome E-Paper Module (296x128, SPI) |
| **Storage** | MicroSD card (SPI / HSPI mode, FAT32 / exFAT with Long File Name support) |
| **Wireless** | Bluetooth Classic A2DP Source (SBC / AAC streaming to headphones and speakers) |
| **Audio Output** | Digital A2DP transmission with gapless track transition |
| **Hardware Controls** | 7 dedicated tactile pushbuttons, 1 rotary encoder with push switch, 1 10K linear potentiometer |
| **Power** | 18650 Li-ion battery (3.7V nominal, 4.2V max) with hardware ADC voltage monitoring |

---

## Hardware Pin Connections

### Display (WeAct Studio 2.9" E-Paper)

| Signal | ESP32 GPIO | Description |
| :--- | :--- | :--- |
| **SCK / CLK** | GPIO 18 | VSPI Clock |
| **DIN / MOSI** | GPIO 23 | VSPI Data Out |
| **CS** | GPIO 5 | Chip Select (Active Low) |
| **DC** | GPIO 22 | Data / Command Selection |
| **RST** | GPIO 21 | Hardware Reset |
| **BUSY** | GPIO 4 | Busy Status (Active High) |

### MicroSD Card (SD-SPI Mode)

| Signal | ESP32 GPIO | Description |
| :--- | :--- | :--- |
| **SCK** | GPIO 14 | HSPI Clock |
| **MOSI** | GPIO 13 | HSPI Data Out |
| **MISO** | GPIO 12 | HSPI Data In |
| **CS** | GPIO 15 | Chip Select (Active Low) |
| **CD** | GPIO 27 | Card Detect Switch |

### Physical Pushbuttons

| Button | ESP32 GPIO | Electrical Config | Function |
| :--- | :--- | :--- | :--- |
| **UP** | GPIO 32 | Internal pull-up to 3.3V | Now Playing: Loop mode (Off / All / One) • Lists: Move cursor up |
| **DOWN** | GPIO 33 | Internal pull-up to 3.3V | Now Playing: Shuffle mode (On / Off) • Lists: Move cursor down |
| **LEFT** | GPIO 25 | Internal pull-up to 3.3V | Now Playing: Previous track • Lists: Navigate left column |
| **RIGHT** | GPIO 26 | Internal pull-up to 3.3V | Now Playing: Next track • Lists: Navigate right column |
| **ENTER** | GPIO 19 | Internal pull-up to 3.3V | Select item / Play-Pause / Toggle Bluetooth connection |
| **BLUETOOTH**| GPIO 2 | Internal pull-up to 3.3V | Dedicated hardware shortcut to Bluetooth screen |
| **BACK** | GPIO 0 | Internal pull-up to 3.3V | Return to previous screen / close menu |

### Rotary Encoder & Analog Volume

| Control | Signal | ESP32 GPIO / ADC | Description |
| :--- | :--- | :--- | :--- |
| **Rotary Encoder** | Phase A | GPIO 36 (Sensor VP) | Quadrature Phase A input |
| **Rotary Encoder** | Phase B | GPIO 39 (Sensor VN) | Quadrature Phase B input |
| **Rotary Switch** | Switch | Shared / GPIO 19 | Play/Pause toggle or Enter |
| **Volume Pot** | Wiper | GPIO 34 (ADC1_CH6) | Digital volume attenuation (0-100%) |
| **Battery Sense** | Divider | GPIO 35 (ADC1_CH7) | 2:1 resistive divider for battery voltage |

---

## Architecture Overview

```text
Core 1 (Real-Time Audio)
┌─────────────────────────────────────────────────────────────┐
│ Audio Pipeline Task (Priority 6)                            │
│  - Decoders (FLAC, MP3, AAC, Opus, WAV)                     │
│  - 128 KB PSRAM Ring Buffer                                 │
│  - Audio Resampler & TPDF 24-to-16-bit Dither               │
│  - Digital Volume Scaler (Smooth non-linear curve)          │
│  - High-Bitpool A2DP Source Audio Streaming                 │
└─────────────────────────────────────────────────────────────┘

Core 0 (System, UI & Storage)
┌──────────────────────────────┐  ┌───────────────────────────┐
│ UI & Rendering Task (Prio 2) │  │ Input Polling Task (Prio 3│
│  - State-Driven Screens      │  │  - Button Debouncing      │
│  - 296x128 1-bit Canvas      │  │  - Encoder Velocity/Seek  │
│  - Partial / Full Refresh    │  │  - Potentiometer Smoothing│
└──────────────────────────────┘  └───────────────────────────┘
┌──────────────────────────────┐  ┌───────────────────────────┐
│ Bluetooth Stack (Bluedroid)  │  │ Storage & Library Task    │
│  - Classic A2DP Source       │  │  - Binary Index Database  │
│  - AVRCP Controller          │  │  - Metadata Extractor     │
│  - Auto-reconnect via NVS    │  │  - Background File Scan   │
└──────────────────────────────┘  └───────────────────────────┘
```

---

## Screen Reference

The graphical interface corresponds directly to `Screens_UI.pdf`:

1. **Now Playing Screen (Page 1)**:
   - 120x120 Floyd-Steinberg dithered album artwork on the left.
   - Top status row: Bluetooth rune icon, connected device name (e.g. `OnePlus Bullets Wireless Z2`), battery capsule with numerical percentage.
   - Center: Track title and artist name in clear sans-serif typography.
   - Progress bar: Elapsed time (`0:01`), progress line with scrub handle, and track duration (`3:34`).
   - Transport icon: Circular Play / Pause symbol.
   - Up Next label: Upcoming track title displayed below transport controls.
   - Right margin: Speaker icon, vertical volume bar, and volume percentage.

2. **Library Screen (Page 2)**:
   - Header: `LIBRARY`
   - Numbered categories and playlists: `1. Favourite Songs`, `2. DJ`, `3. Knight Ride`, `4. Durga Puja Slow`, `5. Shaadi Slow`, `6. Truck Driver`, `7. Gym`.
   - Selection indicator: Rounded rectangle border enclosing the focused item.

3. **Song List Screen (Page 3)**:
   - Header: Category name (e.g. `FAVOURITE SONGS`).
   - Two-column numbered layout showing 14 songs per screen page.
   - Left and right navigation between columns, vertical scrolling between rows.

4. **Bluetooth Screen (Page 4)**:
   - Header: `BLUETOOTH` with real-time status indicator (`OFF`, `Scanning...`, `Connecting...`, `Connected`, `Disconnected`).
   - Numbered list of paired and discovered devices (`1. Stone 1208`, `2. OnePlus Bullets Wireless Z2`, `3. Ayon`).
   - Connected device displays a status checkmark (`✓`) beside the device name.
   - Pressing **ENTER** toggles connection state.
   - Pressing the physical **BLUETOOTH** button returns directly to the active screen.

5. **Alert & Status Screens (Pages 5, 6, 7)**:
   - **SD Card Missing (Page 5)**: Centered SD card icon with `SD CARD NOT FOUND`.
   - **Power Off (Page 6)**: Centered power symbol with `POWER OFF`.
   - **Low Battery (Page 7)**: Centered battery alert symbol with `LOW BATTERY`.

---

## Project Structure

```text
├── CMakeLists.txt
├── sdkconfig.defaults
├── partitions.csv
├── main/
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── hardware_pins.hpp
│   ├── app_state.hpp
│   └── app_state.cpp
└── components/
    ├── audio/          # Audio pipeline, ring buffer, volume scaling
    ├── bluetooth/      # Bluetooth Classic A2DP source and discovery
    ├── decoder/        # FLAC, MP3, AAC, Opus, WAV streaming decoders
    ├── display/        # WeAct 2.9" E-Paper driver, canvas, font engine, icons
    ├── input/          # Button debouncer, encoder acceleration, potentiometer ADC
    ├── library/        # Binary indexed database, search, and playlist manager
    ├── metadata/       # ID3v1, ID3v2, Vorbis Comments, MP4 atoms, artwork loader
    ├── playback/       # Queue, playback controller, NVS resume state
    ├── power/          # Battery voltage monitor, deep sleep, idle timer
    ├── storage/        # FatFS SD-SPI mounting and hotplug detection
    └── ui/             # Screen implementations matching Screens_UI.pdf
```

---

## Build and Flash

### Requirements

- ESP-IDF v5.0 or newer
- ESP32-WROVER-E DevKitC board
- MicroSD card formatted with FAT32 containing audio files inside `/Music`

### Compilation

```bash
idf.py set-target esp32
idf.py build
```

### Flashing and Serial Monitor

```bash
idf.py -p COM_PORT flash monitor
```

---

## Music Library Synchronization

A Python utility is provided in `sync_playlists.py` to synchronize YouTube Music playlists into organized folders directly on your MicroSD card or local directory:

```text
Selected Folder/
├── Playlist Name/
│   ├── Track Name.flac
│   ├── cover.jpg
│   └── .download_archive.txt
```

### Installation

```bash
pip install -r requirements.txt
```

### Usage

1. Open `sync_playlists.py` and add your playlist names and YouTube Music links directly to the `PLAYLISTS` dictionary:

```python
PLAYLISTS: Dict[str, str] = {
    "Favourite Songs": "https://music.youtube.com/playlist?list=...",
    "DJ": "https://music.youtube.com/playlist?list=...",
}
```

2. Run the script:

```bash
python sync_playlists.py
```

To specify a custom output directory (e.g. your MicroSD card mounted on drive `E:`):

```bash
python sync_playlists.py --output E:/Music
```

### Features

- **Folder Hierarchy**: Generates `<Selected Folder>/<Playlist Name>/<Track Name>.<ext>` matching the ESP32 player library scanner.
- **Audio Quality**: Extracts the best available audio stream losslessly to FLAC (`--format flac`, default) without compression loss, or copies raw streams (`--format best`).
- **Metadata and Artwork**: Embeds artist, album, title, and track number tags, embeds cover art into each file, and exports `cover.jpg` for folder-level preview on the 2.9-inch E-Paper screen.
- **Incremental Sync**: Maintains `.download_archive.txt` in each folder so existing tracks are skipped on future runs.
