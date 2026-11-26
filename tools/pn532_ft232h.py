# /// script
# dependencies = ["pyftdi>=0.55"]
# ///
"""Read an ISO14443A card UID from the PN532 module over I2C through an
Adafruit FT232H and pyftdi, with no microcontroller and no Arduino library.

Run:  uv run tools/pn532_ft232h.py [seconds]

Module: switches ch1 ON, ch2 OFF (I2C), unplug and replug its USB after any
switch change. I2C address 0x24.

Wiring (module pad, front silkscreen, to FT232H):
  VCC -> 3V    GND -> GND    SCL -> D0    SDA -> D1 and D2 tied together
With an FT232H breakout that has a Qwiic / STEMMA QT port, a Qwiic cable does
the same job (black GND, red 3.3 V, blue SDA, yellow SCL); a Qwiic-to-female
cable puts the female ends on the module's header pins.

The script sends GetFirmwareVersion, SAMConfiguration, then polls
InListPassiveTarget until a card answers or the time runs out.
"""
import sys
import time

from pyftdi.i2c import I2cController, I2cNackError

ADDR = 0x24
URL = "ftdi://ftdi:232h/1"


def frame(data):
    """Wrap a PN532 command (command byte plus parameters) in a host frame."""
    ln = len(data) + 1
    lcs = (-ln) & 0xFF
    dcs = (-(0xD4 + sum(data))) & 0xFF
    return bytes([0x00, 0x00, 0xFF, ln, lcs, 0xD4]) + bytes(data) + bytes([dcs, 0x00])


def wait_ready(port, timeout):
    """Poll the status byte until the module reports ready (bit 0)."""
    t = time.time()
    while time.time() - t < timeout:
        try:
            if port.read(1)[0] & 0x01:
                return True
        except I2cNackError:
            pass
        time.sleep(0.01)
    return False


def command(port, data, resp_len=24, timeout=2.0):
    """Send a command, check the ACK frame, return the response frame or None."""
    f = frame(data)
    for _ in range(5):  # the first write to a sleeping module can be NACKed
        try:
            port.write(f)
            break
        except I2cNackError:
            time.sleep(0.05)
    else:
        raise RuntimeError("module never ACKed the write (switches, wiring or power-cycle?)")
    if not wait_ready(port, timeout):
        raise RuntimeError("no ready byte after the write")
    ack = port.read(7)  # status byte, then the 6-byte ACK frame
    if ack[1:] != bytes([0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00]):
        raise RuntimeError(f"bad ACK frame: {ack.hex(' ')}")
    if not wait_ready(port, timeout):
        return None  # still working, for example no card in the field yet
    return port.read(resp_len)[1:]  # drop the status byte


def main():
    seconds = float(sys.argv[1]) if len(sys.argv) > 1 else 20
    i2c = I2cController()
    i2c.configure(URL, frequency=100e3)
    port = i2c.get_port(ADDR)
    try:
        r = command(port, [0x02])  # GetFirmwareVersion
        if r is None or r[6] != 0x03:
            raise RuntimeError("no GetFirmwareVersion response")
        # response: 00 00 FF LEN LCS D5 03 IC VER REV SUPPORT DCS 00
        print(f"PN5{r[7]:02X} firmware {r[8]}.{r[9]}")
        command(port, [0x14, 0x01, 0x14, 0x01])  # SAMConfiguration, normal mode
        print(f"Hold a card on the antenna ({seconds:g} s)...")
        end = time.time() + seconds
        while time.time() < end:
            r = command(port, [0x4A, 0x01, 0x00], timeout=1.0)  # InListPassiveTarget
            # response: 00 00 FF LEN LCS D5 4B NBTG TG ATQA(2) SAK NFCIDLEN NFCID...
            if r and r[6] == 0x4B and r[7] >= 1:
                n = r[12]
                print("UID:", r[13:13 + n].hex(" ").upper())
                return
        print("No card found")
        sys.exit(1)
    finally:
        i2c.terminate()


main()
