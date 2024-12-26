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
#define motorTrig        17

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

#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)
#define WAKEUP_GPIO_27 GPIO_NUM_27     // Only RTC IO are allowed
#define WAKEUP_GPIO_12 GPIO_NUM_12     // Only RTC IO are allowed
uint64_t bitmask = BUTTON_PIN_BITMASK(WAKEUP_GPIO_27);
// | BUTTON_PIN_BITMASK(WAKEUP_GPIO_12);
RTC_DATA_ATTR int bootCount = 0;

String cmdFromMsg = "nothing";
String reply;

int bat;

int flowRate = 0;              // Flow rate in ml/s
int totalVolume = 0;           // Total volume in milliliters
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

void print_GPIO_wake_up(){
  int GPIO_reason = esp_sleep_get_ext1_wakeup_status();
  Serial.print("GPIO that triggered the wake up: GPIO ");
  Serial.println((log(GPIO_reason))/log(2), 0);
}
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:     
      Serial.println("Wakeup caused by external signal using RTC_IO");
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      Serial.println("Wakeup caused by external signal using RTC_CNTL");
      print_GPIO_wake_up();
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("Wakeup caused by timer");
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      Serial.println("Wakeup caused by touchpad");
      break;
    case ESP_SLEEP_WAKEUP_ULP:
      Serial.println("Wakeup caused by ULP program");
      break;
    default:
      Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
      break;
  }
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
TaskHandle_t DoCmd;
TaskHandle_t DoReceive;
TaskHandle_t DoCalculate;
TaskHandle_t DoFlush;

float currentWeight;
float previousWeight;

bool doCalculateFlag = false;
bool doFlushFlag = false;

int readBattery(){
  //battery need the adc value
  int batt = movingAverageBatt.addValue(analogRead(adcBatt));
  // Serial.print("batt: ");
  // Serial.println(batt);
  return batt;
}

//core1
void doReceive(void* pvParameters) {
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = 30/ portTICK_PERIOD_MS;
  xLastWakeTime = xTaskGetTickCount ();

  for (;;) {
    if(receivedFlag) {
      receivedFlag = false;
      receivedMsg();
    }
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

//core0
void doCmd(void* pvParameters){

  TickType_t xLastWakeTime;
  const TickType_t xFrequency = 10/ portTICK_PERIOD_MS;
  xLastWakeTime = xTaskGetTickCount ();

  for(;;) {
    
    FlushButton.update();
    LimitSwitch1.update();
    LimitSwitch2.update();

    if(cmdFromMsg == "nothing"){
      // do nothing
      // Serial.println("masuk-nothing");
    }

    else if (cmdFromMsg == "wake") {
      Serial.println("masuk wake");
      doSend("wakeReply"); //battery
      cmdFromMsg = "nothing";
    } 

    else if(cmdFromMsg == "measure"){
      scale.power_up();
      Serial.println("masuk measure");
      doCalculateFlag = true;
      cmdFromMsg = "nothing";
    }


    else if(cmdFromMsg == "stop"){
      Serial.println("masuk stop");
      scale.power_down();
      doCalculateFlag = false;
      doSend("stopReply"); //sensor datas
      cmdFromMsg = "nothing";
    }

    else if (cmdFromMsg == "flush") {
 
      doFlushFlag = true;
      cmdFromMsg = "nothing";
    }

    else if (cmdFromMsg == "sleep"){
      Serial.println("masuk sleep");
      cmdFromMsg = "nothing";
      int state = radio.startReceive();
      doSend("sleepReply");
      delay(1000);
      esp_deep_sleep_start();
    }

    if (FlushButton.isFlagChanged()){
      Serial.println("masuk flush");
      doFlushFlag = true;
      cmdFromMsg = "nothing";
    }

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}


void doCalculate(void* pvParameters){

  
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = 1000/ portTICK_PERIOD_MS;
  xLastWakeTime = xTaskGetTickCount ();

  for (;;){
    if(doCalculateFlag){
      
      currentWeight = scale.get_units(10);
      float weightChange = currentWeight - previousWeight;
      flowRate = weightChange * density;
      if (flowRate > 0) {
          totalVolume += flowRate;
      }
      previousWeight = currentWeight;
      Serial.print(flowRate);
      Serial.print(", ");
      Serial.println(totalVolume);
      
      doSend("data"); //sensor datas 
    }
    else {
      scale.power_down();
    }
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

int state = 0; // State variable for non-blocking execution
void doFlush(void* pvParameters){
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = 100/ portTICK_PERIOD_MS;
  xLastWakeTime = xTaskGetTickCount ();
  for(;;){
    if(doFlushFlag){
      switch (state) {
          case 0:
              // Forward to toss
              if (!LimitSwitch1.isFlagChanged()) {
                  doMotor(1, 255);

                  Serial.println("motor is turning");
              } else {
                  doMotor(0, 0); // Stop motor
                  state = 1;     // Move to next state
              }
              break;

          case 1:
              Serial.println("motor stop and delaying");
              if (nonBlockingDelay(3000)) {
                  state = 2; // Move to reverse
              }

              break;

          case 2:
              // Reverse
              if (!LimitSwitch2.isFlagChanged()) {
                  Serial.println("motor is reversing");
                  doMotor(-1, 255);

              } else {
                  doMotor(0, 0); // Stop motor
                  state = 3;     // Move to next state
              }
              break;

          case 3:
              Serial.println("motor stop and delaying and relay on");
              digitalWrite(relayValve, HIGH);
              if (nonBlockingDelay(3000)) {
                digitalWrite(relayValve, LOW);
                  Serial.println("relay off");
                  state = 4;
              }

              break;

          case 4:
              if (!LimitSwitch1.isFlagChanged()) {
                  doMotor(1, 255);
                  Serial.println("motor is turning again");
              } else {
                  doMotor(0, 0); // Stop motor
                  state = 5;     // Move to next state
              }
              break;

          case 5:
              // Non-blocking delay after reverse motion
              Serial.println("motor stop and delaying");
              if (nonBlockingDelay(3000)) {
                  state = 6; // Activate relay
              }

              break;

          case 6:
              // Reverse to complete cycle
              if (!LimitSwitch2.isFlagChanged()) {
                  doMotor(-1, 255);
                  Serial.println("motor is reversing again FINAL!");
              } else {
                  doMotor(0, 0); // Stop motor
                  Serial.println("done");
                  state = 0;     // Reset state to start over
                  doFlushFlag = false;
                  doSend("flushReply");
              }
              break;
      }
    }
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void setup() {
  Serial.begin(115200);
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  print_wakeup_reason();
  esp_sleep_enable_ext1_wakeup(bitmask, ESP_EXT1_WAKEUP_ANY_HIGH);
  rtc_gpio_pulldown_en(WAKEUP_GPIO_27);
  rtc_gpio_pullup_dis(WAKEUP_GPIO_27);
  rtc_gpio_pulldown_en(WAKEUP_GPIO_12);
  rtc_gpio_pullup_dis(WAKEUP_GPIO_12);
 
  xTaskCreatePinnedToCore(
    doCmd,   /* Task function. */
    "doCmd", /* name of task. */
    5000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    3,           /* priority of the task */
    &DoCmd,      /* Task handle to keep track of created task */
    0);          /* pin task to core 0 */

  xTaskCreatePinnedToCore(
    doFlush,   /* Task function. */
    "doFlush", /* name of task. */
    5000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    2,           /* priority of the task */
    &DoFlush,      /* Task handle to keep track of created task */
    0);          /* pin task to core 0 */
  
  xTaskCreatePinnedToCore(
    doCalculate,   /* Task function. */
    "doCalculate", /* name of task. */
    5000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    2,           /* priority of the task */
    &DoCalculate,      /* Task handle to keep track of created task */
    0);          /* pin task to core 0 */

  xTaskCreatePinnedToCore(
    doReceive,   /* Task function. */
    "doReceive", /* name of task. */
    5000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    3,           /* priority of the task */
    &DoReceive,      /* Task handle to keep track of created task */
    1);          /* pin task to core 1 */

  

  FlushButton.begin();
  LimitSwitch1.begin();
  LimitSwitch2.begin();

  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(pwmA, OUTPUT);
  pinMode(motorTrig, OUTPUT);
  pinMode(relayValve, OUTPUT);
  // pinMode(ledMerah, OUTPUT);
  pinMode(ledHijau, OUTPUT);
  // pinMode(ledIndicator, OUTPUT);
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
  // delay(1000);

  // esp_deep_sleep_start();
  // Serial.println("go sleep......");

}



//calculate ml/s and V


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
int delayInterval = 3000;



void doSend(String which){
  String msg; //flowrate,volume,battery,reply in one packet

  if (which == "wakeReply"){
    msg = String(0)+ "," + String(0) + "," + String(readBattery()) + ","+ "woke!"; 
  }

  else if (which == "data") {
    msg = String(flowRate) + "," + String(totalVolume) + "," + String(readBattery()) + ","+ String(0);
  }

  else if (which == "flushReply") {
    msg = String(0)+"," + String(0) + "," + String(0) + ","+ "flushed!";
  }
  else if (which == "stopReply") {
    msg = String(0)+"," + String(0) + "," + String(0) + ","+ "stopped!";
  }
  else if (which == "sleepReply") {
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

    digitalWrite(motorTrig, HIGH);
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  }
  else if (dir == -1){
    digitalWrite(motorTrig, HIGH);
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  }
  else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    digitalWrite(motorTrig, LOW);
  }
  analogWrite(pwmA, pwm);
}

void receivedMsg(){
    String str;
    int state = radio.readData(str);
    parseMessage(str, cmdFromMsg, reply);
    if (state == RADIOLIB_ERR_NONE) {
      // packet was successfully received
      // Serial.println(F("[SX1278] Received packet!"));

      // print data of the packet
      // Serial.print(F("[SX1278] Data:\t\t"));
      Serial.print(cmdFromMsg);
      Serial.print(" ");
      Serial.println(reply);

      // // print RSSI (Received Signal Strength Indicator)
      // Serial.print(F("[SX1278] RSSI:\t\t"));
      // Serial.print(radio.getRSSI());
      // Serial.println(F(" dBm"));

      // // print SNR (Signal-to-Noise Ratio)
      // Serial.print(F("[SX1278] SNR:\t\t"));
      // Serial.print(radio.getSNR());
      // Serial.println(F(" dB"));

      // // print frequency error
      // Serial.print(F("[SX1278] Frequency error:\t"));
      // Serial.print(radio.getFrequencyError());
      // Serial.println(F(" Hz"));

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

bool parseMessage(String message, String &cmdFromMsg, String &reply) {
  int firstComma = message.indexOf(',');

  // Ensure all commas are found
  if (firstComma == -1) {
    return false;
  }

  cmdFromMsg = message.substring(0, firstComma);
  reply = message.substring(firstComma + 1);

  return true; // Parsing successful
}


//task priority
// receive button core 1
// doCalculate
// doSend
// doFlush

void loop(){

}


