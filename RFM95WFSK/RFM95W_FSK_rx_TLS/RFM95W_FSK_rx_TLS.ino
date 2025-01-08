#include <RadioLib.h>

#define MY_SCLK  14//18
#define MY_MISO  33//12 //19
#define MY_MOSI  13//13 //23
#define MY_CS    32//32

#define SYNC_WORD_INTERRUPT_PIN  12  // Pin number for interrupt syncword ready
 
SPIClass SPI_2(VSPI);
SPISettings spiSettings(2000000, MSBFIRST, SPI_MODE0);
SX1276 radio = new Module(32, 27, 26, 33, SPI_2, spiSettings);

volatile bool interruptFlagSyncWord = false;

#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void interruptHandlerSyncWord() {
  interruptFlagSyncWord = true;
}



void setup() {

  SPI_2.begin(MY_SCLK, MY_MISO, MY_MOSI, MY_CS);
  Serial.begin(9600);


  int state = radio.beginFSK(915.0, 4.8, 5.0, 125.0, 10, 16, true);
  // radio.setCrcFiltering(false);

  pinMode(interruptFlagSyncWord, INPUT_PULLDOWN);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }
  delay(500);

  radio.setSequencerStop();
  radio.setlistenSequence();
  // delay(2000);
  radio.setSequencerStart();
  attachInterrupt(digitalPinToInterrupt(SYNC_WORD_INTERRUPT_PIN), interruptHandlerSyncWord, RISING);

  
 
}


byte byteArr[8];

void loop() {
  Serial.println(radio.getIRQFlags(), BIN);

  if (interruptFlagSyncWord) {
    Serial.println("SyncWord interrupt!");
    delay(50);
    interruptFlagSyncWord = false;  // Reset the flag

    int numBytes = radio.getPacketLength();
    int state = radio.readData(byteArr, numBytes);

    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("[SX1262] Received packet!"));
      Serial.print(F("[SX1262] Data:\t"));

      for(int i=0; i<sizeof(byteArr); i++){
        Serial.print(byteArr[i], HEX);
      }
      Serial.println();

    
    } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
      Serial.println(F("[SX1262] Timed out while waiting for packet!"));
    } else {
      Serial.println(F("[SX1262] Failed to receive packet, code "));
      Serial.println(state);
    }
  }

}

