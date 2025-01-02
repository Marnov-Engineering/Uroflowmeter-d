#include "RadioLib.h"


#define MY_SCLK 14  //18
#define MY_MISO 33  //12 //19
#define MY_MOSI 13  //13 //23
#define MY_CS 32    //32

SPIClass SPI_2(VSPI);

SPISettings spiSettings(2000000, MSBFIRST, SPI_MODE0);

SX1276 radio = new Module(32, 27, 26, 33, SPI_2, spiSettings);


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

int flowRate;
int totalVolume;
int battery;
String reply;


void setup() {

  SPI_2.begin(MY_SCLK, MY_MISO, MY_MOSI, MY_CS);
  Serial.begin(9600); 

  int state = radio.beginFSK(915.0, 4.8, 5.0, 125.0, 10, 16, true);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
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
}



bool parseMessage(String message, int &flowRate, int &totalVolume, int &battery, String &reply) {
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


int counter;
int count;

String inputString = "";  // Variable to store the input string
bool toTransmit = false;  // Flag to indicate when the string is complete

bool nonBlockingDelay(unsigned long duration) {
    static unsigned long startMillis = 0;
    static bool inDelay = false;

    if (!inDelay) {
        startMillis = millis(); // Capture the start time
        inDelay = true;         // Start the delay
    }

    if (millis() - startMillis >= duration) {
        inDelay = false; // Delay finished
        return true;     // Indicate the delay has ended
    }

    return false; // Delay ongoing
}

bool continueSendFlag = false;
String msg;

void loop() {

  while (Serial.available()) {
    char inChar = (char)Serial.read();  // Read the next character
    if (inChar == '\n') {  // Check for newline character (end of input)
      toTransmit = true;
      break;
    }
    inputString += inChar;  // Append the character to the string
  }

  if(toTransmit){

    Serial.println("sending");
    msg = inputString;
    int state = radio.transmit(msg);
    

    // byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
    //                   0x89, 0xAB, 0xCD, 0xEF};
    // int state = radio.transmit(byteArr, 8);

    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(msg + " sent");
    } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
      Serial.println(F("[SX1278] Packet too long!"));
    } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
      Serial.println(F("[SX1278] Timed out while transmitting!"));
    } else {
      Serial.println(F("[SX1278] Failed to transmit packet, code "));
      Serial.println(state);
    }

    toTransmit = false;
    continueSendFlag = true;
    inputString = "";
    reply = "";
    battery = 0;
    state = radio.startReceive();

  }

  //if haven't receive reply yet
  if(nonBlockingDelay(500) && continueSendFlag){
      int state = radio.transmit(msg);
    

    // byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
    //                   0x89, 0xAB, 0xCD, 0xEF};
    // int state = radio.transmit(byteArr, 8);

    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(msg + " sent again");
    } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
      Serial.println(F("[SX1278] Packet too long!"));
    } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
      Serial.println(F("[SX1278] Timed out while transmitting!"));
    } else {
      Serial.println(F("[SX1278] Failed to transmit packet, code "));
      Serial.println(state);
    }
  }
  // Serial.println(radio.getIRQFlags(), BIN);

 if(receivedFlag) {
    receivedFlag = false;
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
      Serial.println(str);

      // // print data of the packet
      Serial.print("flow: ");
      Serial.println(flowRate);
      Serial.print(" total volume: ");
      Serial.println(totalVolume);
      Serial.print(" battery: ");
      Serial.println(battery);
      Serial.print(" reply: ");
      Serial.println(reply);
      
      if(reply == "woke!" || reply == "flushing!" || reply == "measuring!" || reply == "stopped!" || reply == "slept!"){
        continueSendFlag = false;
        // inputString = "";
        // Serial.println("masuk reply =");
      }

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
}
