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
  #ifndef PN532_WIRE
  #define PN532_WIRE Wire   // use -DPN532_WIRE=Wire1 for a second I2C bus (e.g. Qwiic on the Nano R4)
  #endif
  #include <PN532_I2C.h>
  PN532_I2C pn532if(PN532_WIRE);
#else
  #include <SPI.h>
  #include <PN532_SPI.h>
  PN532_SPI pn532if(SPI, 10);
#endif
#include <PN532.h>
#include <NfcAdapter.h>

NfcAdapter nfc = NfcAdapter(pn532if);

void setup() {
      Serial.begin(115200);
    while (!Serial && millis() < 4000);
      Serial.println("NDEF Writer");
      nfc.begin();
}

void loop() {
    Serial.println("\nPlace a formatted Mifare Classic NFC tag on the reader.");
    if (nfc.tagPresent()) {
        NdefMessage message = NdefMessage();
        message.addUriRecord("http://arduino.cc");

        bool success = nfc.write(message);
        if (success) {
          Serial.println("Success. Try reading this tag with your phone.");        
        } else {
          Serial.println("Write failed.");
        }
    }
    delay(5000);
}