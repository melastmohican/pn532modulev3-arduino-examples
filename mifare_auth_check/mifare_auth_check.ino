// Diagnostic: try standard published default Mifare Classic keys on block 4 of a card
// you own, using HSU on Serial1. Prints which key/type authenticates.
#include <PN532_HSU.h>
#include <PN532.h>

PN532_HSU pn532hsu(Serial1);
PN532 nfc(pn532hsu);

struct NamedKey { const char *name; uint8_t k[6]; };
const NamedKey keys[] = {
  {"factory FFFFFFFFFFFF", {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}},
  {"MAD A0A1A2A3A4A5",     {0xA0,0xA1,0xA2,0xA3,0xA4,0xA5}},
  {"NDEF D3F7D3F7D3F7",    {0xD3,0xF7,0xD3,0xF7,0xD3,0xF7}},
  {"zeros 000000000000",   {0x00,0x00,0x00,0x00,0x00,0x00}},
  {"B0B1B2B3B4B5",         {0xB0,0xB1,0xB2,0xB3,0xB4,0xB5}},
};

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 4000);
  nfc.begin();
  if (!nfc.getFirmwareVersion()) { Serial.println("No PN532"); while (1) delay(1000); }
  nfc.setPassiveActivationRetries(0x10);
  nfc.SAMConfig();
  Serial.println("Mifare auth check ready");
}

void loop() {
  Serial.println("\n--- hold a Mifare Classic card on the antenna ---");
  for (uint8_t t = 0; t < 2; t++) {
    for (const NamedKey &nk : keys) {
      uint8_t uid[7], len;
      if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &len)) {
        Serial.println("no card"); delay(500); continue;
      }
      uint8_t key[6]; memcpy(key, nk.k, 6);
      uint8_t ok = nfc.mifareclassic_AuthenticateBlock(uid, len, 4, t, key);
      Serial.print("\n>>> block 4, key"); Serial.print(t ? "B " : "A ");
      Serial.print(nk.name); Serial.println(ok ? "  = AUTH OK" : "  = fail");
      delay(200);
    }
  }
  delay(3000);
}
