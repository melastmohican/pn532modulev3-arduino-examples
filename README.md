# Arduino with clone ELECHOUSE PN532 NFC NXP RFID Module V3

## Install the libraries

None of these are in the Arduino Library Manager. The `PN532` library used here is [elechouse/PN532](https://github.com/elechouse/PN532) (the Seeed library with an added HSU driver), branch `PN532_HSU` (the repository's default branch, so a plain "Download ZIP" gets it, and the unzipped folder is named `PN532-PN532_HSU` after the branch), and it ships the NDEF library in the same zip. Other places in this repo call it "the Seeed library", and the `seeed_examples/` folder is named after the examples it came from; it is this same library.

1. Download [https://github.com/elechouse/PN532/archive/PN532_HSU.zip](https://github.com/elechouse/PN532/archive/PN532_HSU.zip) and unzip it (a folder `PN532-PN532_HSU`).
2. Copy these folders from it into your sketchbook's `libraries` folder: `PN532`, `PN532_HSU` and `NDEF` (add `PN532_I2C` and `PN532_SPI` only if you want those interfaces). The sketchbook location is in Arduino IDE under Settings (default `~/Documents/Arduino/libraries`). Do not copy the whole `PN532-PN532_HSU` folder, and do not rename the folders. Restart the IDE.
3. Apply the one patch (needed on the RP2040/RP2350 core, which rejects upstream's `PN532_HSU.cpp`; harmless elsewhere), from the `libraries` folder:
   ```
   cd ~/Documents/Arduino/libraries      # your sketchbook libraries folder
   patch -p1 < /path/to/this/repo/pn532-hsu-minimal-patch.diff
   ```
   It changes `PN532_HSU/PN532_HSU.cpp` only; add `--dry-run` first to check. Details are in the "Library patch" section below.
4. Check: `arduino-cli compile -b <fqbn> seeed_examples/iso14443a_uid` should build.

Without step 2 for `NDEF` the NDEF examples fail with a missing `NfcAdapter.h`. As an alternative to copying it, point the build at the downloaded copy: `--library /path/to/PN532-PN532_HSU/NDEF` (that is what the commands in this README do on this machine, where `NDEF` is not installed in the sketchbook).

## Choosing the interface (HSU, I2C or SPI)

Every sketch starts with one setting:

```
#ifndef PN532_INTERFACE
#define PN532_INTERFACE 1     // 1 = HSU (Serial1), 2 = I2C (Wire), 3 = SPI (SS on pin 10)
#endif
```

Change the number, or override it at build time: `arduino-cli compile ... --build-property "compiler.cpp.extra_flags=-DPN532_INTERFACE=2" <sketch>`. HSU is the default and the only interface tested on hardware so far. All 18 sketches compile with HSU and with I2C on the Feather RP2350 HSTX and the Nano Matter (72 builds). The SPI branch does not compile with the current `PN532_SPI` library on either board: it calls `setDataMode`, `setBitOrder` and `setClockDivider`, which the SPI classes of these cores no longer have. SPI would also need header pins soldered to the module, so it is on hold. The module's switches must match the chosen interface (HSU: both OFF, I2C: ch1 ON and ch2 OFF), and it needs a full USB replug after any change.

## HSU mode on Adafruit Feather RP2350 HSTX (verified)

Reads an NTAG-type tag, a Mifare Classic 1K card and a Mifare Classic 1K key fob.
The fob must be close to the antenna loop.

**Switches:** both OFF (HSU). I2C is channel 1 ON, channel 2 OFF.

| Module pad (back silkscreen) | Feather |
|---|---|
| VCC | 3V (5V/USB also works) |
| GND | GND |
| TXD | RX (GPIO1) |
| RXD | TX (GPIO0) |

HSU shares the same header pads as I2C (SDA/SCL on the front, TXD/RXD on the back).

**Sketch:** `seeed_examples/iso14443a_uid/iso14443a_uid.ino` (the Seeed library's `iso14443a_uid` example, with four changes: HSU branch on, `NfcAdapter.h` include dropped, wait for `Serial` and a retry loop instead of halting when the PN532 is not found, retries `0xFF` to `0x10`)
- Set the HSU branch to `#elif 1` (uses `Serial1`).
- Use `nfc.setPassiveActivationRetries(0x10)`. With `0xFF` (infinite) every poll timed out, even with a card in place.
- Board: `rp2040:rp2040:adafruit_feather_rp2350_hstx`, default options.

**Library patch (bisected 2026-11-22):** the only change needed is in `PN532_HSU/PN532_HSU.cpp`: cast the constants passed to `_serial->write()` to `uint8_t`, because upstream does not compile on the RP2040/RP2350 core (`call of overloaded 'write(int)' is ambiguous`). Diff: [pn532-hsu-minimal-patch.diff](pn532-hsu-minimal-patch.diff). Apply to a fresh download with `patch -p1 -d <library folder> < pn532-hsu-minimal-patch.diff`.

With only that patch, the Feather read a Mifare Classic card on every poll, both after flashing and after a cold USB replug.

Some extra edits I tried at first (full diff in [pn532-hsu-library-patches.diff](pn532-hsu-library-patches.diff)) were **not needed** on this setup: ACK wait 10 ms to 1000 ms; `wakeup()` with 14 zero bytes, `flush()` and `delay(50)`; a `0x55 0x55` plus 14 zero-byte prefix before every frame; `flush()` after the last byte. They were reverted: the library used for all the results here has only the casts patch (the diff is kept for reference). I did not test them on the UNO R4, where HSU failed anyway.

`PN532/PN532_debug.h` is unchanged from upstream (`DEBUG` commented out). Remove the `//` to see `Write:` / `Ack:` / `Read:` frames.

**Gotchas**
- Unplug and replug USB after any wire change; the module does not recover otherwise.
- The Arduino IDE Serial Monitor holds the port and blocks `arduino-cli upload`; close it first.
- `Ack: Timeout` with debug on, or `Didn't find PN53x board`, means no HSU link: check switches, then TXD/RXD.

**Not working: Arduino UNO R4 WiFi.** HSU gets no ACK in either TX/RX orientation, and an I2C scan finds nothing, although the UNO's `Serial1` loopback passes and the same module works on the Feather. Suspected cause (unproven): the UNO's 5V logic against the module's 3.3V output; a level shifter is the untested fix.


**Working: Arduino Nano Matter** (2026-11-22, `SiliconLabs:silabs:nano_matter`, 3.3 V logic). `seeed_examples/iso14443a_uid` compiles unchanged (929 KB, 59% of flash) and reads the white Mifare card (UID 50 61 78 58) on the first try.

| PN532 pad (HSU name) | Nano Matter pin |
|---|---|
| VCC | 3V3 |
| GND | GND |
| TXD | D1 (`Serial1` RX, PA5) |
| RXD | D0 (`Serial1` TX, PA4) |

Both switches OFF, full USB replug after wiring. The core's `Serial1` TX is D0, the reverse of the Nano's usual `TX`/`RX` silkscreen, so wire by the pin names D0/D1.

Also verified on the Nano Matter, with no code changes:
- NDEF `ReadTag` on the white card (Mifare Classic, not NDEF formatted) and on the NTAG (both Zephyr records), `WriteTag` and `RestoreZephyr` on the NTAG. `ReadTag` on the blue fob (Mifare Classic, UID F0 88 7F 5C) printed its `http://arduino.cc` record once it was pushed hard against the board. `FormatTag` formatted the blank white card ("Success, tag formatted as NDEF." on the first pass, then "Unable to authenticate block 0" on later passes, as on the Feather). `WriteTagMultipleRecords` wrote three records to the white card (read back with `ReadTag`: Text `Hello, Arduino!`, URI `http://arduino.cc`, Text `Goodbye, Arduino!`), `EraseTag` emptied it and `CleanTag` restored it to factory state. One mistake worth knowing: the first `FormatTag` run was by accident against the already-formatted fob, which refused authentication and was left unchanged. Build with the `NDEF` library path as for the Feather.
- `seeed_examples/p2p_raw` against the M5 unit in both directions (`snep_receive.py` printed `hello world`; `snep_send.py` got SNEP Success and the PN532 printed `hello from M5 to Nano`).
- `seeed_examples/p2p_with_ndef_library` (send only; the read branch is compiled out): the M5 unit's `snep_receive.py` printed `uri: http://www.seeedstudio.com`, a 31-byte message built with the NDEF library.
- `seeed_examples/emulate_tag_ndef`: the PN532 as a Type 4 tag with the URI `http://www.elechouse.com` (UID `12 34 56`, which the PN532 shows as `08 12 34 56`). An iPhone with NFC Tools read the URI (it labels the tag "ISO 7816 / Fujitsu", which is meaningless). The M5 unit's `detect` found it as `ATQA 0400, SAK 0x20` (ISO-DEP), and the M5 repo's `read-ndef` command (Type 4 support added 22 Nov 2026) printed `uri: http://www.elechouse.com`.

The NTAG (UID 04 64 31 66 D3 2A 81) and the blue fob only read at the right spot on the antenna: held loosely they were missed, then found by sweeping. The module's antenna is a loop on one part of the board, not under its centre, and small coils couple poorly with a few centimetres of offset.

**Not working: Arduino Nano R4** (`arduino:renesas_uno:nanor4`, 5 V logic, same RA4M1 chip as the UNO R4). Silent ("Didn't find PN53x board") with the wiring that matches its pin table (module TXD to D0 = RX, RXD to D1 = TX, VCC 3V3), although a `Serial1` loopback on D0/D1 passes. Together with the UNO R4 this fits the 3.3 V against 5 V logic idea (the two 3.3 V boards work, the two 5 V boards do not), but it is unproven; a level shifter on the module's TXD to D0 is the untested fix. Note that `Serial1` is TX = D0 on the Nano Matter but TX = D1 on the Nano R4.

## NDEF examples over HSU (verified on Feather RP2350 HSTX)

The `NDEF` library (`don/NDEF`) examples are in [ndef_examples/](ndef_examples/), switched from I2C/SPI to HSU on `Serial1` at 115200. If `NDEF` is not copied into your sketchbook (see "Install the libraries"), build against the unzipped copy:

```
arduino-cli compile -b rp2040:rp2040:adafruit_feather_rp2350_hstx \
  --library /path/to/PN532-PN532_HSU/NDEF ndef_examples/ReadTag
arduino-cli upload  -b rp2040:rp2040:adafruit_feather_rp2350_hstx \
  -p <port> ndef_examples/ReadTag
```

| Example | Result |
|---|---|
| ReadTag | Verified on NTAG-type tag, Mifare Classic 1K card and key fob |
| ReadTagExtended | Verified on the white card (adds TNF, type, payload hex and string) |
| FormatTag | Verified on the card and the fob |
| WriteTag | Verified on the fob (`http://arduino.cc`) |
| WriteTagMultipleRecords | Verified on the card (3 records, 68 bytes) |
| EraseTag | Verified on the card (back to 1 empty record, 3 bytes) |
| WriteTag on NTAG | Verified (NFC Forum Type 2 tag: wrote `http://arduino.cc`, then restored the Zephyr records with `ndef_examples/RestoreZephyr`) |
| CleanTag | Verified on the card (back to factory keys; ReadTag shows "not NDEF formatted" again) |

The Seeed `PN532` library examples are copied to [seeed_examples/](seeed_examples/) with HSU selected, and all compile for the Feather. Build with `--library /path/to/PN532-PN532_HSU/NDEF` (needed by `emulate_tag_ndef` and `p2p_with_ndef_library`). Not copied: `android_hce` and the two FeliCa examples (no Android app or FeliCa card in hand). Edits: `p2p_raw` and `p2p_with_ndef_library` rewritten from `PN532_SPI` to `PN532_HSU(Serial1)`. Run on hardware: `iso14443a_uid` and `p2p_raw` (Feather and Nano Matter), `p2p_with_ndef_library` (send only) and `emulate_tag_ndef` (Nano Matter only). Compile-tested only, not run: `mifareclassic_formatndef`, `mifareclassic_memdump`, `mifareclassic_ndeftoclassic`, `mifareclassic_updatendef` and `readMifare` (the NDEF examples in `ndef_examples/` cover the same ground and were run). `emulate_tag_ndef` and the `p2p_*` examples need an NFC phone or a second NFC device.

**Tag types in hand**
- NTAG-type, NFC Forum Type 2, UID `04 64 31 66 D3 2A 81`: holds 2 records (text `hello from zephyr`, URI `https://zephyrproject.org`).
- Mifare Classic 1K card, UID `50 61 78 58`: couples at normal distance.
- Mifare Classic 1K key fob, UID `F0 88 7F 5C`: works only when pushed hard against the board (small coil).

**Gotchas**
- **FormatTag succeeds only on its first pass.** After that, every loop prints `Unable to authenticate block 0 to enable card formatting!` and `Format failed.`, because a formatted card no longer has the factory key in sector 0. That does not mean the card is bad. Check with ReadTag.
- A serial reader that opens after boot misses the first pass. Let the sketch loop and watch for repeated output.
- Authentication was checked directly with `mifare_auth_check/mifare_auth_check.ino` (tries standard default keys on block 4). On a formatted card, block 4 authenticates with Key A `D3 F7 D3 F7 D3 F7` and Key B `FF FF FF FF FF FF`.
- The NDEF library does not set the passive-activation retries; if polling ever times out with a card in range, see the retries note above.
- Close the Arduino IDE Serial Monitor before uploading or reading from the command line; it holds the port.


## PN532 vs M5Stack NFC unit (ST25R3916) interop (verified 2026-11-22)

The M5 unit is the tag, the PN532 (on the Feather, HSU) is the reader. This is a reader-to-tag link between the two devices, not peer-to-peer.

**Setup:** the M5 unit runs on an FT232H, using the [m5nfc-st25r3916-experiments](https://github.com/melastmohican/m5nfc-st25r3916-experiments) repo (its `python/` folder; clone it first). Hold the PN532 antenna flat against the M5 unit for the whole test.

1. Flash the PN532 side (`ndef_examples/ReadTag`, or `WriteTag` for the write test) as in the NDEF section above.
2. Start the M5 unit as an NTAG215 (UID `04112233445566`) for 40 to 60 seconds:

```
cd m5nfc-st25r3916-experiments/python   # your clone of the repo
uv run python tools/listen_probe.py 45 t2
```

| Test | PN532 side | M5 side |
|---|---|---|
| Read | ReadTag prints `NFC Forum Type 2`, UID `04 11 22 33 44 55 66`, 1 empty record | 36 frames answered; READ `30 03/04/05` answered with capability container `E1 10 3E 00` and `03 00 FE 00` |
| Write | WriteTag prints `Success` | 72 frames answered, 56 tag writes, decoded `uri: http://arduino.cc` |

**Limits**
- Real P2P (NFC-DEP, LLCP, SNEP, i.e. `seeed_examples/p2p_raw`) cannot be tested: the M5 stack has reader and tag-emulation modes (`t2`, `t4`, `sniff`) but no NFC-DEP.
- `t4` (Type 4 tag) and `sniff` were not tried. The NDEF library does not read Type 4 tags.
- Close the Arduino IDE Serial Monitor first; run the M5 listener and the PN532 reader at the same time, with the units already together.


## Real peer-to-peer: the M5 unit and the PN532 exchange SNEP messages (verified 2026-11-22)

The PN532 runs `seeed_examples/p2p_raw` (flashed on the Feather) as an NFC-DEP target and SNEP client. The M5 unit is the NFC-DEP initiator and the LLCP/SNEP server. Scripts in the [M5 repo](https://github.com/melastmohican/m5nfc-st25r3916-experiments) (`python/tools/`):

| Script | What it does |
|---|---|
| `dep_probe.py` | Select the peer (SAK `0x40`), ATR_REQ/ATR_RES, DEP_REQ/DEP_RES carrying LLCP SYMM, answers S(RTOX), RLS_REQ |
| `snep_receive.py` | Adds the LLCP server (SYMM, CONNECT to `urn:nfc:sn:snep` answered with CC, I and RR PDUs, DISC answered with DM) and a SNEP Put receiver with Continue and Success; decodes the NDEF with `nfc.ndef` |
| `snep_send.py` | The other direction: the M5 unit is the LLCP client. SYMM to tell the PN532's phase, CONNECT to SAP 4 by name, one SNEP Put in one I PDU, polls with SYMM until the Success arrives, answers with RR |

```
cd m5nfc-st25r3916-experiments/python   # your clone of the repo
uv run python tools/dep_probe.py 4       # link-layer check
uv run python tools/snep_receive.py 60   # receive the SNEP Put
uv run python tools/snep_send.py "hello from the unit"   # send one (the PN532 must be in its read phase)
```

Result: `media text/plain: 'hello world'`, the 24-byte NDEF message baked into `p2p_raw`, received on 2 of 2 runs. `snep_send.py` got SNEP Success and the PN532 printed `get a SNEP message:` and `...T.enhello from the unit` on its serial port (a 26-byte Text record), on the Feather and on the Nano Matter.

Notes
- `p2p_raw` alternates a write phase (PN532 is the SNEP client and sends CONNECT and the Put) and, after a 3 s delay, a read phase (PN532 is a SNEP server and waits for the peer to CONNECT). `snep_receive.py` only serves the write phase, so it releases and retries when the peer sends only SYMM; it succeeded on attempt 4 (about 20 s) both times.
- The PN532 asks for a timeout extension (RTOX x7) on almost every exchange because its host is busy; the initiator must echo it.
- `snep_send.py` needs the PN532's read phase and retries until it lands in it. A PN532 that stops answering needs a full USB power cycle (both scripts then fail with "no response from card"). Sending is limited to one I PDU (the sketch reads 128 bytes).
- Not done: ports of peer-to-peer to the MicroPython, Arduino and Rust implementations of the M5 repo.

## Origins and licences

The sketches in `seeed_examples/`, `ndef_examples/` and `Adafruit-PN532/` are modified copies of examples from the Seeed/Elechouse PN532 library, Don Coleman's NDEF library and Adafruit's Adafruit-PN532 library, all BSD 3-clause. Their notices are in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). The files written for this repository (`I2C_Scan/`, `mifare_auth_check/`, `ndef_examples/RestoreZephyr/`, this README and the patches) are under the BSD 3-clause licence in [LICENSE](LICENSE). The Elechouse manual and schematic PDFs are not included here; see the links under References.

## References
https://www.elechouse.com/product/pn532-nfc-rfid-module-v4/
https://www.elechouse.com/elechouse/images/product/PN532_module_V3/PN532_%20Manual_V3.pdf
https://www.elechouse.com/elechouse/images/product/PN532_module_V3/PN532_shematic_drowing.pdf
https://github.com/elechouse/pn532
https://www.elechouse.com/compare/pn532-vs-pn7160-vs-st25r3916/
https://www.elechouse.com/pn532-tester/ho
https://www.elechouse.com/docs/pn532-v4/
https://www.elechouse.com/solutions/best-nfc-module-for-arduino/
https://crates.io/search?q=PN532
https://github.com/WMT-GmbH/pn532
https://github.com/don/NDEF
https://github.com/Seeed-Studio/PN532
https://learn.adafruit.com/adafruit-pn532-rfid-nfc
https://github.com/adafruit/Adafruit-PN532/
https://github.com/playfultechnology/arduino-rfid-PN532
https://www.instructables.com/RC522-and-PN532-RFID-Basics/
https://www.circuits-diy.com/interfacing-pn532-nfc-rfid-module-with-arduino
https://techatronic.com/rfid-arduino-tutorial/
https://circuitstoday.com/interfacing-rfid-with-arduino
https://www.instructables.com/Arduino-RFID-Smart-Door-Tutorial/
https://randomnerdtutorials.com/arduino-time-attendance-system-with-rfid/
https://projecthub.arduino.cc/coolcat231/simple-rfid-tutorial-d6f10e
https://how2electronics.com/arduino-pn532-nfc-based-payment-entrance-security-system/
https://sertit.no/certified-products/product-archive/fm1280-v05-dual-interface-smart-card-chip-with-ic-dedicated-software
https://arduinogetstarted.com/tutorials/arduino-rfid-nfc
https://github.com/FilipRokita/arduino-nfc
https://www.instructables.com/RFID-NFC-ARDUINO-ACCESS-CONTROL-SYSTEM/
https://www.modulaindustries.co.za/post/5-awesome-pn532-nfc-projects-a-complete-arduino-walkthrough-with-code
https://wiki.seeedstudio.com/XIAO-BLE-Sense-NFC-Usage/
https://www.dfrobot.com/product-1917.html
https://wiki.dfrobot.com/dfr0231-h/
https://github.com/DFRobot/DFRobot_PN532
https://github.com/krulkip/PN532Toolkit
https://www.makerfabs.com/pn532-nfc-module-v3.html
https://www.amazon.com/dp/B0GTNRDXYB
https://manuals.plus/asin/B09BTH8GGJ
https://hiletgo.com/ProductDetail/2156958.html
https://github.com/jmgjmg/PN532
https://www.amazon.com/dp/B01I1J17LC/
https://github.com/adafruit/Adafruit-PN532
https://github.com/krulkip/PN532Toolkit
