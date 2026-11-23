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
  Serial.println("NDEF Reader");
  nfc.begin();
}

void loop(void) {
  Serial.println("\nScan a NFC tag\n");

  if (nfc.tagPresent())
  {
    NfcTag tag = nfc.read();
    Serial.println(tag.getTagType());
    Serial.print("UID: ");Serial.println(tag.getUidString());

    if (tag.hasNdefMessage()) // every tag won't have a message
    {

      NdefMessage message = tag.getNdefMessage();
      Serial.print("\nThis NFC Tag contains an NDEF Message with ");
      Serial.print(message.getRecordCount());
      Serial.print(" NDEF Record");
      if (message.getRecordCount() != 1) {
        Serial.print("s");
      }
      Serial.println(".");

      // cycle through the records, printing some info from each
      int recordCount = message.getRecordCount();
      for (int i = 0; i < recordCount; i++)
      {
        Serial.print("\nNDEF Record ");Serial.println(i+1);
        NdefRecord record = message.getRecord(i);
        // NdefRecord record = message[i]; // alternate syntax

        Serial.print("  TNF: ");Serial.println(record.getTnf());
        Serial.print("  Type: ");Serial.println(record.getType()); // will be "" for TNF_EMPTY

        // The TNF and Type should be used to determine how your application processes the payload
        // There's no generic processing for the payload, it's returned as a byte[]
        int payloadLength = record.getPayloadLength();
        byte payload[payloadLength];
        record.getPayload(payload);

        // Print the Hex and Printable Characters
        Serial.print("  Payload (HEX): ");
        PrintHexChar(payload, payloadLength);

        // Force the data into a String (might work depending on the content)
        // Real code should use smarter processing
        String payloadAsString = "";
        for (int c = 0; c < payloadLength; c++) {
          payloadAsString += (char)payload[c];
        }
        Serial.print("  Payload (as String): ");
        Serial.println(payloadAsString);

        // id is probably blank and will return ""
        String uid = record.getId();
        if (uid != "") {
          Serial.print("  ID: ");Serial.println(uid);
        }
      }
    }
  }
  delay(3000);
}
