#include <RadioLib.h>

#define MY_SCLK  14//18
#define MY_MISO  33//12 //19
#define MY_MOSI  13//13 //23
#define MY_CS    32//32

#define PREAMBLE_INTERRUPT_PIN  35  // Pin number for interrupt onpreamble
#define RX_READY_INTERRUPT_PIN  34  // Pin number for interrupt rx ready
#define SYNC_WORD_INTERRUPT_PIN  18  // Pin number for interrupt syncword ready
 
SPIClass SPI_2(VSPI);
SPISettings spiSettings(2000000, MSBFIRST, SPI_MODE0);
SX1276 radio = new Module(32, 27, 26, 33, SPI_2, spiSettings);

volatile bool interruptFlag = false;
volatile bool interruptFlagRx = false;
volatile bool interruptFlagSyncWord = false;

#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif

void interruptHandler() {
  interruptFlag = true;
}

#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif

void interruptHandlerRx() {
  interruptFlagRx = true;
}

#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif

void interruptHandlerSyncWord() {
  interruptFlagSyncWord = true;
}

void setup() {

  SPI_2.begin(MY_SCLK, MY_MISO, MY_MOSI, MY_CS);
  Serial.begin(9600);
  int state = radio.beginFSK(915.0, 4.8, 5.0, 125.0, 10, 120, true);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }
  delay(500);
  state = radio.setSpreadingFactor(7);

  radio.setPacketReceivedAction(interruptHandlerRx);

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


  radio.setSequencerStop();
  radio.setSequences();
  // delay(2000);
  radio.setSequencerStart();
// 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




  // radio.setPacketReceivedAction(setFlag);

  attachInterrupt(digitalPinToInterrupt(PREAMBLE_INTERRUPT_PIN), interruptHandler, RISING);
  // attachInterrupt(digitalPinToInterrupt(RX_READY_INTERRUPT_PIN), interruptHandlerRx, RISING);
  attachInterrupt(digitalPinToInterrupt(SYNC_WORD_INTERRUPT_PIN), interruptHandlerSyncWord, RISING);
 
}



void loop() {
  Serial.println("IRQ: ");
  Serial.println(radio.getIRQFlags(), BIN);
  // Serial.println(radio.getIrqFlags1(), BIN);
  // Serial.println(123);

  if (interruptFlag) {
    Serial.println("Rising, preamble detected !");
    interruptFlag = false;  // Reset the flag
    delay(500);

    // radio.setSequences();
    // radio.setSequencerStop();
    // radio.getRegSeqVal();

  }

  if (interruptFlagRx) {
    Serial.println("RX ready !");
    radio.setSequencerStop();
    delay(1000);
    interruptFlagRx = false;  // Reset the flag

  }

  if (interruptFlagSyncWord) {
    Serial.println("SyncWord detected !");
    interruptFlagSyncWord = false;  // Reset the flag
    delay(1000);

  }

}

//   // set the function that will be called
//   // when LoRa preamble is not detected within CAD timeout period
//   radio.setDio0Action(setFlagTimeout, RISING);

//   // set the function that will be called
//   // when LoRa preamble is detected
//   radio.setDio1Action(setFlagDetected, RISING);

//   // start scanning the channel
//   Serial.print(F("[SX1278] Starting scan for LoRa preamble ... "));
//   state = radio.startChannelScan();
//   if (state == RADIOLIB_ERR_NONE) {
//     Serial.println(F("success!"));
//   } else {
//     Serial.print(F("failed, code "));
//     Serial.println(state);
//   }
// }

// // flag to indicate that a preamble was not detected
// volatile bool timeoutFlag = false;

// // flag to indicate that a preamble was detected
// volatile bool detectedFlag = false;

// // this function is called when no preamble
// // is detected within timeout period
// // IMPORTANT: this function MUST be 'void' type
// //            and MUST NOT have any arguments!
// #if defined(ESP8266) || defined(ESP32)
//   ICACHE_RAM_ATTR
// #endif
// void setFlagTimeout(void) {
//   // we timed out, set the flag
//   timeoutFlag = true;
// }

// // this function is called when LoRa preamble
// // is detected within timeout period
// // IMPORTANT: this function MUST be 'void' type
// //            and MUST NOT have any arguments!
// #if defined(ESP8266) || defined(ESP32)
//   ICACHE_RAM_ATTR
// #endif
// void setFlagDetected(void) {
//   // we got a preamble, set the flag
//   detectedFlag = true;
// }

// void loop() {
//   // check if we need to restart channel activity detection
//   if(detectedFlag || timeoutFlag) {
//     // check if we got a preamble
//     if(detectedFlag) {
//       // LoRa preamble was detected
//       Serial.println(F("[SX1278] Preamble detected!"));
//     } else {
//       // nothing was detected
//       Serial.println(F("[SX1278] Channel free!"));
//     }
    
//     // start scanning the channel
//     Serial.print(F("[SX1278] Starting scan for LoRa preamble ... "));

//     // start scanning current channel
//     int state = radio.startChannelScan();
//     if (state == RADIOLIB_ERR_NONE) {
//       Serial.println(F("success!"));
//     } else {
//       Serial.print(F("failed, code "));
//       Serial.println(state);
//     }

//     // reset flags
//     timeoutFlag = false;
//     detectedFlag = false;
//   }
// }

