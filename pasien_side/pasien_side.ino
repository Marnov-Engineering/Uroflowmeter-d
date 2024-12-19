#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#include "HX711.h"
#include "soc/rtc.h"
#include "RadioLib.h"

#include "driver/rtc_io.h"

#define in1              19
#define in2              18
#define in3              5
#define pwmA             23
#define motorTrig        22

#define relayValve       21

#define dtHx711          16
#define sckHx711         4

#define sclkLora         14
#define misoLora         33
#define mosiLora         13
#define csLora           32
#define dio0Lora         27
#define rstLora          26
#define dio4Lora         12

#define limitSwitch1     2
#define limitSwitch2     15
#define flushButton      34

#define ledMerah         35
#define ledHijau         25
#define ledIndicator     36

#define adcBatt          39

#define WAKEUP_GPIO_27 GPIO_NUM_27     // Only RTC IO are allowed
#define WAKEUP_GPIO_12 GPIO_NUM_12     // Only RTC IO are allowed
uint64_t bitmask = BUTTON_PIN_BITMASK(WAKEUP_GPIO_27) | BUTTON_PIN_BITMASK(WAKEUP_GPIO_12);
RTC_DATA_ATTR int bootCount = 0;

String cmdFromMsg;

int bat;

float flowRate = 0;              // Flow rate in ml/s
float totalVolume = 0;           // Total volume in milliliters
float density = 1.0;             // Density of the liquid 

SPIClass SPI_2(VSPI);
SPISettings spiSettings(2000000, MSBFIRST, SPI_MODE0);
SX1276 radio = new Module(csLora, dio0Lora, rstLora, misoLora, SPI_2, spiSettings);

volatile bool receivedFlag = false;
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // we got a packet, set the flag
  receivedFlag = true;
  // Serial.println("interrupt");
}

class MovingAverage {
  public:
    MovingAverage(int windowSize) {
      if (windowSize <= 0) {
        Serial.println("window size is not valid.");
        return;
      }
      this->windowSize = windowSize;
      this->sum = 0;
      this->count = 0;
      this->index = 0;

      this->values = new float[windowSize];

    }

    ~MovingAverage() {
      delete[] values;
    }

    float addValue(float value) {

      if (count == windowSize) {
        sum -= values[index];
      } else {
        count++;
      }

      values[index] = value;
      sum += value;
      index = (index + 1) % windowSize;

      return sum / count;
    }

    void setWindowSize(int newWindowSize) {
      if(windowSize<newWindowSize) {
        _manipulate(newWindowSize);
      }
      else {
        windowSize = newWindowSize;
        sum = 0;
        count = 0;
        index = 0;
      }

    }

    int getWindowSize() {
      return windowSize;
    }


  private:
    int windowSize;
    float* values;
    float sum;
    int count;
    int index;

    void _manipulate(int newWindowSizeManipulate){
      float sumPerCount =  (sum/count);

      sum = sumPerCount * newWindowSizeManipulate;

      for(int i = count; i <= newWindowSizeManipulate; i++) {
        values[i] = sumPerCount;
      }

      count = newWindowSizeManipulate;
      windowSize = newWindowSizeManipulate;
    }
};

class DebounceButton {
  private:
      int pin;                     // Button pin
      unsigned long debounceDelay; // Debounce time in milliseconds
      unsigned long lastDebounceTime; // Last time the button state was updated
      int buttonState;             // Current stable button state
      int lastReading;             // Last raw reading from the button
      bool flag;                   // Custom flag
      bool flagChanged;            // Flag to track if the flag was toggled

  public:
      // Constructor
      DebounceButton(int buttonPin, unsigned long delay = 50) 
          : pin(buttonPin), debounceDelay(delay), lastDebounceTime(0), 
            buttonState(HIGH), lastReading(HIGH), flag(false), flagChanged(false) {}

      // Initialize the button pin
      void begin() {
          pinMode(pin, INPUT_PULLUP); // Assuming a pull-up resistor setup
      }

      // Update the button state (to be called in loop)
      void update() {
          int reading = digitalRead(pin);

          if (reading != lastReading) {
              lastDebounceTime = millis();
          }

          if ((millis() - lastDebounceTime) > debounceDelay) {
              if (reading != buttonState) {
                  buttonState = reading;

                  // Toggle the flag on a button press
                  if (buttonState == LOW) {
                      flag = !flag;       // Toggle the flag
                      flagChanged = true; // Mark that the flag was changed
                  }
              }
          }

          lastReading = reading;
      }

      // Check if the button is pressed
      bool isPressed() const {
          return buttonState == LOW;
      }

      // Check if the button is released
      bool isReleased() const {
          return buttonState == HIGH;
      }

      // Get the flag value
      bool getFlag() const {
          return flag;
      }

      // Check if the flag was toggled
      bool isFlagChanged() {
          if (flagChanged) {
              flagChanged = false; // Reset the flagChanged state
              return true;
          }
          return false;
      }
};

MovingAverage movingAverageBatt(10);
DebounceButton FlushButton(flushButton);
DebounceButton LimitSwitch1(limitSwitch1);
DebounceButton LimitSwitch2(limitSwitch2);
HX711 scale;
StaticJsonDocument<200> doc;

void setup() {
  Serial.begin(115200);

  esp_sleep_enable_ext1_wakeup_io(bitmask, ESP_EXT1_WAKEUP_ANY_HIGH);
  rtc_gpio_pulldown_en(WAKEUP_GPIO_2);
  rtc_gpio_pullup_dis(WAKEUP_GPIO_2);
  rtc_gpio_pulldown_en(WAKEUP_GPIO_15);
  rtc_gpio_pullup_dis(WAKEUP_GPIO_15);

  xTaskCreatePinnedToCore(
    updateButton,   /* Task function. */
    "updateButton", /* name of task. */
    30000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Task1,      /* Task handle to keep track of created task */
    0);          /* pin task to core 0 */

  FlushButton.begin();

  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(pwmA, OUTPUT);
  pinMode(motorTrig, OUTPUT);
  pinMode(relayValve, OUTPUT);
  pinMode(limitSwitch1, OUTPUT);
  pinMode(limitSwitch2, OUTPUT);
  pinMode(ledMerah, OUTPUT);
  pinMode(ledHijau, OUTPUT);
  pinMode(ledIndicator, OUTPUT);
  pinMode(adcBatt, INPUT);

  rtc_cpu_freq_config_t config;
  rtc_clk_cpu_freq_get_config(&config);
  rtc_clk_cpu_freq_to_config(RTC_CPU_FREQ_80M, &config);
  rtc_clk_cpu_freq_set_config_fast(&config);
  scale.begin(dtHx711, sckHx711);
  scale.set_scale(965.472);
  scale.tare();

  SPI_2.begin(sclkLora, misoLora, mosiLora, csLora);
  
  int state = radio.beginFSK(915.0, 4.8, 5.0, 125.0, 10, 16, true);
  // if (state == RADIOLIB_ERR_NONE) {
  //   Serial.println(F("success!"));
  // } else {
  //   Serial.print(F ("failed, code "));
  //   Serial.println(state);
  //   while (true) { delay(10); }
  // }
  state = radio.setSpreadingFactor(7);

  radio.setPacketReceivedAction(setFlag);
  state = radio.startReceive();

}



//calculate ml/s and V
unsigned long previousMillis = 0;
const unsigned long interval = 1000; 
void doCalculate(){
  if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;


        currentWeight = scale.get_units(10);


        float weightChange = currentWeight - previousWeight;
        flowRate = weightChange * density;

        if (flowRate > 0) {
            totalVolume += flowRate;
        }

        previousWeight = currentWeight;

        // Print results
        // Serial.print("Flow Rate: ");
        // Serial.print(flowRate);
        // Serial.print(" ml/s, Total Volume: ");
        // Serial.print(totalVolume);
        // Serial.println(" ml");
    }
}

//BLOCKING PROCEDURE
void doFlush(){
  
  while(!LimitSwitch1.isFlagChanged()) {doMotor(1, 50)}; //forward to toss
  doMotor(0, 0); //stop motor
  //delay();
  //reverse
  while(!LimitSwitch2.isFlagChanged()) {doMotor(-1, 50)};
  doMotor(0, 0);//stop motor
  //delay();
  //valve relay active high
  digitalWrite(relayValve, HIGH);
  //delay(); //Fill water
  digitalWrite(relayValve, LOW);
  while(!LimitSwitch1.isFlagChanged()) {doMotor(1, 50)};
  // delay();
  while(!LimitSwitch2.isFlagChanged()) {doMotor(-1, 50)};


}



// bool parseMessage(String message, float &flowRate, float &totalVolume, int &battery, String &reply) {
//   int firstComma = message.indexOf(',');
//   int secondComma = message.indexOf(',', firstComma + 1);
//   int thirdComma = message.indexOf(',', firstComma + 2);

//   // Ensure all commas are found
//   if (firstComma == -1 || secondComma == -1 || thirdComma == -1) {
//     return false;
//   }

//   // Extract substrings and convert to respective types
//   temperature = message.substring(0, firstComma).toFloat();
//   humidity = message.substring(firstComma + 1, secondComma).toFloat();
//   lightLevel = message.substring(secondComma + 1).toInt();

//   return true; // Parsing successful
// }


void doSend(String which){
  //send ml/s and V in one packet
  String msg; //flowrate,volume,battery,reply

  if (which = "wakeReply"){
    msg = String(0)+ "," + String(0) + "," + String(readBattery()) + ","+ "woke!";
  }

  else if (which = "data") {
    msg = String(flowRate) + "," + String(totalVolume) + "," + String(readBattery()) + ","+ String(0);
  }

  else if (which = "flushReply") {
    msg = String(0)+"," + String(0) + "," + String(0) + ","+ "flushed!";
  }
  else if (which = "stopReply") {
    msg = String(0)+"," + String(0) + "," + String(0) + ","+ "stopped!";
  }
  else if (which = "sleepReply") {
    msg = String(0)+"," + String(0) + "," + String(0) + ","+ "slept!";
  }
  


  int state = radio.transmit(msg);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(msg + " <<sent");
  } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
    Serial.println(F("[SX1278] Packet too long!"));
  } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
    Serial.println(F("[SX1278] Timed out while transmitting!"));
  } else {
    Serial.println(F("[SX1278] Failed to transmit packet, code "));
    Serial.println(state);
  }


}

void doMotor(int dir, int pwm){
  if(dir == 1) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  }
  else if (dir == -1){
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  }
  else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }
  analogWrite(pwmA, pwm);
}


void receivedMsg(){
    String str;
    int state = radio.readData(str);
    parseMessage(str, float &temperature, float &humidity, int &lightLevel)
    //what do we wanna do to the cmd, assign cmd var with received cmd
    if (state == RADIOLIB_ERR_NONE) {
      // packet was successfully received
      Serial.println(F("[SX1278] Received packet!"));

      // print data of the packet
      Serial.print(F("[SX1278] Data:\t\t"));
      Serial.println(str);

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

    //
    cmdFromMsg = doc["cmd"];
    state = radio.startReceive();
}

float readBattery(){
  //battery
  return movingAverageBatt.addValue(analogRead(adcBatt));
}

void loop(){
  unsigned long currentMillis = millis(); // for doCalculate procedure
  if(receiveFlag){
    receivedFlag = false;
    receivedMsg();
  }
    

  if(cmdFromMsg == "nothing"){
    // do nothing
  }

  else if(cmdFromMsg == "measure"){
    doCalculate();
    doSend("data"); //sensor datas
  }

  else if (cmdFromMsg == "wake") {
    doSend("wakeReply"); //battery
    cmdFromMsg = "nothing";

  }
  else if(cmdFromMsg == "stop"){
    doSend("stopReply"); //sensor datas
    cmdFromMsg = "nothing";
  }

  else if (FlushButton.isFlagChanged() || cmdFromMsg = "flush") {
    receivedFlag = false;
    // Serial.print("Flag toggled to: ");
    // Serial.println(FlushButton.getFlag() ? "ON" : "OFF");
    doFlush();
    doSend("flush reply");
    cmdFromMsg = "nothing";
  }

  else if (cmdFromMsg == "sleep"){
    //loadcell sleep
    cmdFromMsg = "nothing";
    esp_deep_sleep_start();
  }

}

void UpdateButton(void* pvParameters) {

  for (;;) {
    readBattery();
    FlushButton.update();
    LimitSwitch1.update();
    LimitSwitch2.update();

  }
}
