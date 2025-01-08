#include "RadioLib.h"


#define MY_SCLK  14//18
#define MY_MISO  33//12 //19
#define MY_MOSI  13//13 //23
#define MY_CS    32//32

SPIClass SPI_2(VSPI);

SPISettings spiSettings(2000000, MSBFIRST, SPI_MODE0);

SX1276 radio = new Module(32, 27, 26, 33, SPI_2, spiSettings);


void setup() {

  SPI_2.begin(MY_SCLK, MY_MISO, MY_MOSI, MY_CS);
  Serial.begin(9600);



  int state = radio.beginFSK(915.0, 4.8, 5.0, 125.0, 10, 16, true);
  // radio.setCrcFiltering(false);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  state = radio.setBeaconSequence();

  // state = radio.setSpreadingFactor(7);

  // int state = radio.begin(915.0);
  
  // if (state == RADIOLIB_ERR_NONE) {
  //   Serial.println(F("success!"));
  // } else {
  //   Serial.print(F("failed, code "));
  //   Serial.println(state);
  //   while (true) { delay(10); }
  // }
}

int counter;
int count;

void loop() {

  // FSK modem can use the same transmit/receive methods
  // as the LoRa modem, even their interrupt-driven versions
  // NOTE: FSK modem maximum packet length is 63 bytes!

  // transmit FSK packet
  // Serial.println("sending");
  // String msg = "ABCD";

  // int state = radio.transmit(msg);
  
    byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                      0x89, 0xAB, 0xCD, 0xEF};
    int state = radio.transmit(byteArr, 8);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("sent!");
  } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
    Serial.println(F("[SX1278] Packet too long!"));
  } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
    Serial.println(F("[SX1278] Timed out while transmitting!"));
  } else {
    Serial.println(F("[SX1278] Failed to transmit packet, code "));
    Serial.println(state);
  }


  counter++;
  delay(1000);








  //LORA MODEM TRANSMIT


  // Serial.print(F("[SX1278] Transmitting packet ... "));

  // // you can transmit C-string or Arduino string up to
  // // 255 characters long
  // String str = String(count++);
  // int state = radio.transmit(str);

  // // you can also transmit byte array up to 256 bytes long
  // /*
  //   byte byteArr[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
  //   int state = radio.transmit(byteArr, 8);
  // */

  // if (state == RADIOLIB_ERR_NONE) {
  //   // the packet was successfully transmitted
  //   Serial.println(F(" success!"));

  //   // print measured data rate
  //   Serial.print(F("[SX1278] Datarate:\t"));
  //   Serial.print(radio.getDataRate());
  //   Serial.println(F(" bps"));
  // } 

  // else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
  //   // the supplied packet was longer than 256 bytes
  //   Serial.println(F("too long!"));
  // } 

  // else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
  //   // timeout occurred while transmitting packet
  //   Serial.println(F("timeout!"));

  // } 

  // else {
  //   // some other error occurred
  //   Serial.print(F("failed, code "));
  //   Serial.println(state);

  // }

  // // wait for a second before transmitting again
  // delay(1500);


}
