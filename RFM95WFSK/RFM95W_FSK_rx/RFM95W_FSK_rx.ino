#include "RadioLib.h"


#define MY_SCLK  14//18
#define MY_MISO  33//12 //19
#define MY_MOSI  13//13 //23
#define MY_CS    32//32


SPIClass SPI_2(3);

SPISettings spiSettings(2000000, MSBFIRST, SPI_MODE0);

SX1276 radio = new Module(32, 27, 26, 33, SPI_2, spiSettings);


void setup() {

  SPI_2.begin(MY_SCLK, MY_MISO, MY_MOSI, MY_CS);
  Serial.begin(9600);

  int state = radio.beginFSK(915.0, 4.8, 5.0, 125.0, 10, 16, true);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F ("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }
  state = radio.setSpreadingFactor(7);

  radio.setPacketReceivedAction(setFlag);

  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success receive mode!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  String str;
  Serial.println(radio.readData(str));
}
// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // we got a packet, set the flag
  receivedFlag = true;
  // Serial.println("interrupt");
}

float flowRate;
float totalVolume;
int battery;
String reply;

bool parseMessage(String message, float &flowRate, float &totalVolume, int &battery, String &reply) {
  int firstComma = message.indexOf(',');
  int secondComma = message.indexOf(',', firstComma + 1);
  int thirdComma = message.indexOf(',', secondComma + 1);

  // Ensure all commas are found
  if (firstComma == -1 || secondComma == -1 || thirdComma == -1) {
    return false;
  }

  // Extract substrings and convert to respective types
  flowRate = message.substring(0, firstComma).toFloat();
  totalVolume = message.substring(firstComma + 1, secondComma).toFloat();
  battery = message.substring(secondComma + 1, thirdComma).toInt();
  reply = message.substring(thirdComma + 1);

  return true; // Parsing successful
}


void loop() {



// Serial.println(radio.getIRQFlags(), BIN);

 if(receivedFlag) {
    Serial.println("IRQ: ");
    // Serial.println(radio.getIRQFlags(), BIN);
    // Serial.println(radio.getIrqFlags1(), BIN);
    // Serial.println(radio.getIRQFlags(), BIN);
    // reset flag
    receivedFlag = false;

    // you can read received data as an Arduino String
    String str;
    int state = radio.readData(str);

    // you can also read received data as byte array
    /*
      byte byteArr[8];
      int numBytes = radio.getPacketLength();
      int state = radio.readData(byteArr, numBytes);
    */

    if (state == RADIOLIB_ERR_NONE) {
      // packet was successfully received
      Serial.println(F("[SX1278] Received packet!"));

      parseMessage(str, flowRate, totalVolume, battery, reply);

      // print data of the packet
      Serial.print(F("[SX1278] Data:\t\t"));
      Serial.print(flowRate);
      Serial.print(" ");
      Serial.print(totalVolume);
      Serial.print(" ");
      Serial.print(battery);
      Serial.print(" ");
      Serial.println(reply);

      // print RSSI (Received Signal Strength Indicator)
      Serial.print(F("[SX1278] RSSI:\t\t"));
      Serial.print(radio.getRSSI());
      Serial.println(F(" dBm"));

      // print SNR (Signal-to-Noise Ratio)
      Serial.print(F("[SX1278] SNR:\t\t"));
      Serial.print(radio.getSNR());
      Serial.println(F(" dB"));

      // print frequency error
      Serial.print(F("[SX1278] Frequency error:\t"));
      Serial.print(radio.getFrequencyError());
      Serial.println(F(" Hz"));

    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
      Serial.println(F("[SX1278] CRC error!"));

    } else {
      // some other error occurred
      Serial.print(F("[SX1278] Failed, code "));
      Serial.println(state);

    }
    state = radio.startReceive();
  }
  // String str;
  // int state = radio.receive(str);
  // /*
  //   byte byteArr[8];
  //   int state = radio.receive(byteArr, 8);
  // */
  // if (state == RADIOLIB_ERR_NONE) {
  //   Serial.println(F("[SX1278] Received packet!"));
  //   Serial.print(F("[SX1278] Data:\t"));
  //   Serial.println(str);
  // } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
  //   Serial.println(F("[SX1278] Timed out while waiting for packet!"));
  // } else {
  //   Serial.println(F("[SX1278] Failed to receive packet, code "));
  //   Serial.println(state);
  // }
}
