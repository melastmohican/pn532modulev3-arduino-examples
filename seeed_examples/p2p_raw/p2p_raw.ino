// snep_test.ino
// send a SNEP message to adnroid and get a message from android

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

#include "llcp.h"
#include "snep.h"

SNEP nfc(pn532if);

void setup()
{
    Serial.begin(115200);
    Serial.println("-------Peer to Peer--------");
}

uint8_t message[] = {
0xD2, 0xA, 0xB, 0x74,0x65, 0x78, 0x74, 0x2F, 0x70, 0x6C, 
0x61, 0x69, 0x6E, 0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x77, 
0x6F, 0x72, 0x6C, 0x64};

uint8_t buf[128];

void loop()
{

    nfc.write(message, sizeof(message));
    delay(3000);

    int16_t len = nfc.read(buf, sizeof(buf));
    if (len > 0) {
      Serial.println("get a SNEP message:");
      for (uint8_t i = 0; i < len; i++) {
        Serial.print(buf[i], HEX);
        Serial.print(' ');
      }
      Serial.print('\n');
      for (uint8_t i = 0; i < len; i++) {
        char c = buf[i];
        if (c <= 0x1f || c > 0x7f) {
          Serial.print('.');
        } else {
          Serial.print(c);
        }
      }
      Serial.print('\n');
    }
    delay(3000);
}