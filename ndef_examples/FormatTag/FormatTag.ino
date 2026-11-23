// Formats a Mifare Classic tags as an NDEF tag
// This will fail if the tag is already formatted NDEF
// nfc.clean will turn a NDEF formatted Mifare Classic tag back to the Mifare Classic format

// Interface: 1 = HSU (UART on Serial1), 2 = I2C (Wire), 3 = SPI (SS on pin 10).
// Change the number here, or build with -DPN532_INTERFACE=2.
#ifndef PN532_INTERFACE
#define PN532_INTERFACE 1
#endif
#if PN532_INTERFACE == 1
  #include <PN532_HSU.h>
  PN532_HSU pn532if(Serial1);
#elif PN532_INTERFACE == 2
  #include <Wire.h>
  #include <PN532_I2C.h>
  PN532_I2C pn532if(Wire);
#else
  #include <SPI.h>
  #include <PN532_SPI.h>
  PN532_SPI pn532if(SPI, 10);
#endif
#include <PN532.h>
#include <NfcAdapter.h>

NfcAdapter nfc = NfcAdapter(pn532if);

void setup(void) {
    Serial.begin(115200);
    while (!Serial && millis() < 4000);
    Serial.println("NDEF Formatter");
    nfc.begin();
}

void loop(void) {
    
    Serial.println("\nPlace an unformatted Mifare Classic tag on the reader.");
    if (nfc.tagPresent()) {

        bool success = nfc.format();
        if (success) {
          Serial.println("\nSuccess, tag formatted as NDEF.");
        } else {
          Serial.println("\nFormat failed.");
        }

    }
    delay(5000);
}
