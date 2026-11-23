// Erases a NFC tag by writing an empty NDEF message 

#include <PN532_HSU.h>
#include <PN532.h>
#include <NfcAdapter.h>

PN532_HSU pn532hsu(Serial1);
NfcAdapter nfc = NfcAdapter(pn532hsu);

void setup(void) {
    Serial.begin(115200);
    while (!Serial && millis() < 4000);
    Serial.println("NFC Tag Eraser");
    nfc.begin();
}

void loop(void) {
    Serial.println("\nPlace a tag on the NFC reader to erase.");

    if (nfc.tagPresent()) {

        bool success = nfc.erase();
        if (success) {
            Serial.println("\nSuccess, tag contains an empty record.");        
        } else {
            Serial.println("\nUnable to erase tag.");
        }

    }
    delay(5000);
}
