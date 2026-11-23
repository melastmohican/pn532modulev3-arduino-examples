#include "emulatetag.h"
#include "NdefMessage.h"

// Interface: 1 = HSU (UART on Serial1), 2 = I2C (Wire), 3 = SPI (SS on pin 10).
// Change the number here, or build with -DPN532_INTERFACE=2.
#ifndef PN532_INTERFACE
#define PN532_INTERFACE 1
#endif

#if PN532_INTERFACE == 3
  #include <SPI.h>
  #include <PN532_SPI.h>
  #include "PN532.h"

  PN532_SPI pn532spi(SPI, 10);
  EmulateTag nfc(pn532spi);
#elif PN532_INTERFACE == 1
  #include <PN532_HSU.h>
  #include <PN532.h>
      
  PN532_HSU pn532hsu(Serial1);
  EmulateTag nfc(pn532hsu);
#else
  #include <Wire.h>
  #include <PN532_I2C.h>
  #include <PN532.h>

  PN532_I2C pn532i2c(Wire);
  EmulateTag nfc(pn532i2c);
#endif





uint8_t ndefBuf[120];
NdefMessage message;
int messageSize;

uint8_t uid[3] = { 0x12, 0x34, 0x56 };

void setup()
{
  Serial.begin(115200);
  Serial.println("------- Emulate Tag --------");
  
  message = NdefMessage();
  message.addUriRecord("http://www.elechouse.com");
  messageSize = message.getEncodedSize();
  if (messageSize > sizeof(ndefBuf)) {
      Serial.println("ndefBuf is too small");
      while (1) { }
  }
  
  Serial.print("Ndef encoded message size: ");
  Serial.println(messageSize);

  message.encode(ndefBuf);
  
  // comment out this command for no ndef message
  nfc.setNdefFile(ndefBuf, messageSize);
  
  // uid must be 3 bytes!
  nfc.setUid(uid);
  
  nfc.init();
}

void loop(){
    // uncomment for overriding ndef in case a write to this tag occured
    //nfc.setNdefFile(ndefBuf, messageSize); 
    
    // start emulation (blocks)
    nfc.emulate();
        
    // or start emulation with timeout
    /*if(!nfc.emulate(1000)){ // timeout 1 second
      Serial.println("timed out");
    }*/
    
    // deny writing to the tag
    // nfc.setTagWriteable(false);
    
    if(nfc.writeOccured()){
       Serial.println("\nWrite occured !");
       uint8_t* tag_buf;
       uint16_t length;
       
       nfc.getContent(&tag_buf, &length);
       NdefMessage msg = NdefMessage(tag_buf, length);
       msg.print();
    }

    delay(1000);
}
