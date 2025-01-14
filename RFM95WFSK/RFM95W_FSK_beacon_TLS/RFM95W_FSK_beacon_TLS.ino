#include "RadioLib.h"


#define MY_SCLK  14//18
#define MY_MISO  33//12 //19
#define MY_MOSI  13//13 //23
#define MY_CS    32//32

SPIClass SPI_2(VSPI);

SPISettings spiSettings(2000000, MSBFIRST, SPI_MODE0);

SX1276 radio = new Module(32, 27, 26, 33, SPI_2, spiSettings);
volatile bool transmittedFlag = false;
 
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // we sent a packet, set the flag
  transmittedFlag = true;
}

void setup() {

  SPI_2.begin(MY_SCLK, MY_MISO, MY_MOSI, MY_CS);
  Serial.begin(9600);

  int state = radio.beginFSK(915.0, 4.8, 5.0, 125.0, 10, 1000, true);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // radio.setCrcFiltering(false);
  attachInterrupt(digitalPinToInterrupt(27), setFlag, RISING);
  
  state = radio.setSequencerStop();
  state = radio.setBeaconSequence();

  byte byteArr[] = {0xAA, 0xBB, 0xAA, 0xBB,
                  0xAA, 0xBB, 0xAA, 0xBB};

  state = radio.fillFifo(byteArr, 8);
  state = radio.setSequencerStart();
  
  delay(1000);
  Serial.println("sampaii sini");

  // int state = radio.begin(915.0);
  
  // if (state == RADIOLIB_ERR_NONE) {
  //   Serial.println(F("success!"));
  // } else {
  //   Serial.print(F("failed, code "));
  //   Serial.println(state);
  //   while (true) { delay(10); }
  // }
}

void loop() {
  Serial.println(radio.getIRQFlags(), BIN);

  if(transmittedFlag) {
    transmittedFlag = false;
    Serial.println("terkirim");
  }
}
