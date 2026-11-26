// send a NDEF message to adnroid or get a NDEF message
//
// note: [NDEF library](https://github.com/Don/NDEF) is needed.

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

#include "snep.h"
#include "NdefMessage.h"

SNEP nfc(pn532if);
uint8_t ndefBuf[128];

void setup()
{
    Serial.begin(115200);
    Serial.println("-------Peer to Peer--------");
}

void loop()
{
#if 1
    Serial.println("Send a message to Android");
    NdefMessage message = NdefMessage();
    message.addUriRecord("http://www.seeedstudio.com");
    int messageSize = message.getEncodedSize();
    if (messageSize > sizeof(ndefBuf)) {
        Serial.println("ndefBuf is too small");
        while (1) {
        }

    }

    message.encode(ndefBuf);
    if (0 >= nfc.write(ndefBuf, messageSize)) {
        Serial.println("Failed");
    } else {
        Serial.println("Success");
    }

    delay(3000);
#else
    // it seems there are some issues to use NdefMessage to decode the received data from Android
    Serial.println("Get a message from Android");
    int msgSize = nfc.read(ndefBuf, sizeof(ndefBuf));
    if (msgSize > 0) {
        NdefMessage msg  = NdefMessage(ndefBuf, msgSize);
        msg.print();
        Serial.println("\nSuccess");
    } else {
        Serial.println("failed");
    }
    delay(3000);
#endif
}
