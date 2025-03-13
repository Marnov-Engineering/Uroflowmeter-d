#include <TFT_eSPI.h>
#include <lvgl.h>
#include "RadioLib.h"
#include "Adafruit_Thermal.h"

#define TFT_HOR_RES   480
#define TFT_VER_RES   320

#define MY_SCLK  14//18
#define MY_MISO  33//12 //19
#define MY_MOSI  13//13 //23
#define MY_CS    32//32

#define TOTAL_LENGTH 800

SPIClass SPI_2(3);
SPISettings spiSettings(2000000, MSBFIRST, SPI_MODE0);
SX1276 radio = new Module(32, 27, 26, 33, SPI_2, spiSettings);
volatile bool receivedFlag = false;

#define TX_PIN 17
#define RX_PIN 16
Adafruit_Thermal printer(&Serial2); 

uint8_t PROGMEM outputArray[TOTAL_LENGTH];

TFT_eSPI tft = TFT_eSPI(); 

int data1;
int data2;
int x_1 ;
int x_2 ;
int y_1 = 0;
int y_2 = 20;
int x_hasil;
int y_hasil;

int dx;
int dy;
int d1;
int d2;
int p;
float m;
int koordinat;

int stop; // 40 dibagi input
int j = 0;
int nilai;

String msg;
bool toTransmit;
bool continueSendFlag;
uint8_t defaultSyncWord[] = {0x12, 0xAD};
uint8_t wakeSyncWord[] = {0x11, 0xAD};
String inputString = "";
bool bConnecting;

int batt = 100;
int uro = 0;
int titik = 0;
int urutan_data;
int array_flow[90] ;
int array_volum[90];
bool bPage_plotter = false;
bool baca_lora = HIGH;
uint32_t LastTime;
static int simulated_value = 0;
static uint16_t flow_data_count = 0; // Counter for flow rate chart
static uint16_t vol_data_count = 0;  // Counter for volume chart
static bool is_flow_chart_shift_mode = false; // Tracks mode for flow chart
static bool is_vol_chart_shift_mode = false;  // Tracks mode for volume chart
const int potPin = 35;


#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))

float flowRate;
float totalVolume;
int battery;
String reply;

unsigned long lastTickMillis = 0;
lv_obj_t *scaling_rect;
lv_obj_t *batts;
lv_obj_t *screen;
static lv_obj_t * btn1;
static lv_obj_t * btn2;
static lv_obj_t * btn3;
static lv_obj_t * btn_stop;
lv_obj_t *screen_plotter;
lv_obj_t *chart1;
lv_obj_t *chart2;
lv_timer_t *dataflow_timer;
lv_timer_t *datavol_timer;
lv_chart_series_t * ser1;
lv_chart_series_t * ser2;
static lv_subject_t fw_download_percent_subject;
static lv_subject_t fw_update_status_subject;

typedef enum {
    FW_UPDATE_STATE_IDLE,
    FW_UPDATE_STATE_CONNECTING,
    FW_UPDATE_STATE_START_CONNECTING,
    FW_UPDATE_STATE_S_CONNECTING,
    FW_UPDATE_STATE_CONNECTED,
    FW_UPDATE_STATE_DOWNLOADING,
    FW_UPDATE_STATE_CANCEL,
    FW_UPDATE_STATE_READY,
    FW_UPDATE_STATE_START_READY,
    FW_UPDATE_STATE_START_READY_D,
    FW_UPDATE_STATE_START_FINISH,
    FW_UPDATE_STATE_STOP,
} fw_update_state_t;

#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif

void setFlag(void) {
  // we got a packet, set the flag
  receivedFlag = true;
  // Serial.println("interrupt");
}

void update_rectangle_width(int batt) {
    batt = constrain(batt, 0, 100);
    int new_width = map(batt, 0, 100, 0, 100);
    lv_obj_set_size(scaling_rect, new_width, 40);
}

void vLoraSetup(){
  SPI_2.begin(MY_SCLK, MY_MISO, MY_MOSI, MY_CS);
  int state = radio.beginFSK(915.0, 4.8, 5.0, 125.0, 10, 16, true);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F ("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

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

void vReadLora(){
  if(receivedFlag) {
    receivedFlag = false;
    String str;
    int state = radio.readData(str);
    
    if (state == RADIOLIB_ERR_NONE) {
      // packet was successfully received
      Serial.println(F("[SX1278] Received packet!"));

      parseMessage(str, flowRate, totalVolume, battery, reply);
      array_flow[titik] = flowRate;
      array_volum[titik] = totalVolume;
      titik++;
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
}

void convertArray(const int *data, int dataSize, uint8_t *output) {
  // Inisialisasi array output dengan nilai 0x00
  urutan_data = dataSize;
  for(int K = 0; K < urutan_data;K++){
    int j_0 = 0;
    int j_1 = 9;
    int j_2 = 19;
    int j_3 = 29;
    int j_4 = 39;

    for (int i = 0; i < TOTAL_LENGTH; i++) {
    output[i] = 0x00;
    j = i;
    if( j == j_0){
      output[j] = 0xFF;
      j_0 = j_0+40;
    }
    if( j == j_1){
      output[j] = 0x1C;
      j_1 = j_1+40;
    }
    if( j == j_4){
      output[j] = 0x1C;
      j_4 = j_4+40;
    }
    if( j == j_2){
      output[j] = 0x1C;
      j_2 = j_2+40;
    }
    if( j == j_3){
      output[j] = 0x1C;
      j_3 = j_3+40;
    }
  }

    x_1 = data[K];
    if(K == (urutan_data-1) ){
      break;
    }
    else{
      x_2 = data[K+1];
    }

  

  if(x_1 < x_2){
    dx = x_2 - x_1;
    dy = y_2 - y_1;
    m = (float)dy / dx;

    x_hasil = x_1;
    y_hasil = y_1;
    nilai = (x_1+y_1)-1;

    if(m >= 1){
      d1 = 2*dx;
      d2 = 2*(dx-dy);
      p = d1 - dy;
      
      Serial.print("M LEBIH BESAR ");
      for (int i = 1; i <= 20; i++) {
        // Serial.print("Iterasi ke-");
        // Serial.println(i);
        nilai = x_hasil + y_hasil;
        output[nilai] = 0xFF;
        j++;
        // y_hasil = y_hasil/40;

        if ( p >= 0 ){
          p = p+d2;
          x_hasil++;
          y_hasil = y_hasil + 40;
          // Serial.print("x_hasil : ");
          // Serial.println(x_hasil);
          // Serial.print("y_hasil : ");
          // Serial.println(y_hasil);
        }
        else if(p < 0){
          p = p+d1;
          y_hasil = y_hasil + 40;
          // Serial.print("x_hasil : ");
          // Serial.println(x_hasil);
          // Serial.print("y_hasil : ");
          // Serial.println(y_hasil);
        }
      }

    }
    else if(m < 1){
      d1 = 2*dy;
      d2 = 2*(dx-dy);
      p = d1 - dx;
      // nilai = x_hasil+y_hasil;
      // output[nilai] = 0xFF;

      Serial.print("M LEBIH KECIL ");
      for (int i = 1; i <= x_2; i++) {
        // Serial.print("Iterasi ke-");
        // Serial.println(i);
        nilai = x_hasil+y_hasil;
        output[nilai] = 0xFF;
        // y_hasil = y_hasil/40;
        // if (i == 1){
        //   y_hasil = y_hasil - 40;
        // }
        // if(x_hasil == data2 ){
        //   break;
        // }
        if(y_hasil >= 760){
          Serial.print("break");
          break;
          i = x_2;
        }
        if ( p >= 0 ){
          p = p-d2;
          x_hasil++;
          y_hasil = y_hasil + 40;
          // Serial.print("x_hasil : ");
          // Serial.println(x_hasil);
          // Serial.print("nilai : ");
          // Serial.println(nilai);
        }
        else if(p < 0){
          p = p+d1;
          x_hasil++;
          // y_hasil = y_hasil + 40;
          // Serial.print("x_hasil : ");
          // Serial.println(x_hasil);
          // Serial.print("nilai : ");
          // Serial.println(nilai);
        }
      }

    }

  }

  else{
        dx = x_1 - x_2;
        dy = y_2 - y_1;
        m = (float)dy / dx;

        x_hasil = x_1;
        y_hasil = y_1;
        nilai = (x_1+y_1)-1;

        if(m >= 1){
          d1 = 2*dx;
          d2 = 2*(dx-dy);
          p = d1 - dy;
          Serial.print("M LEBIH BESAR MINUS");
              for (int i = 1; i <= 40; i++) {
              // Serial.print("Iterasi ke-");
              // Serial.println(i);
              // nilai = x_hasil+y_hasil;
              if(y_hasil >= 800){
                break;
                Serial.print("BREAK ");
              }
              output[nilai] = 0xFF;
              j++;
              // y_hasil = y_hasil/40;

              if ( p >= 0 ){
                p = p+d2;
                x_hasil--;
                y_hasil = y_hasil + 40;
                // Serial.print("x_hasil : ");
                // Serial.println(x_hasil);
                // Serial.print("y_hasil : ");
                // Serial.println(y_hasil);
                
              }
            else if(p < 0){
              p = p+d1;
              y_hasil = y_hasil + 40;
              // Serial.print("x_hasil : ");
              // Serial.println(x_hasil);
              // Serial.print("y_hasil : ");
              // Serial.println(y_hasil);
              
            }
            nilai = x_hasil+y_hasil;
          } 
        }
        else if(m < 1){
          d1 = 2*dy;
          d2 = 2*(dx-dy);
          p = d1 - dx;
          // nilai = x_hasil+y_hasil;
          // output[nilai] = 0xFF;
          Serial.print("M LEBIH KECIL minus ");
          for (int i = 1; i <= 40; i++) {
            // Serial.print("Iterasi ke-");
            // Serial.println(i);
            
            
            output[nilai] = 0xFF;
            // y_hasil = y_hasil/40;
            // if (i == 1){
            //   y_hasil = y_hasil - 40;
            // }
            if(y_hasil >= 760){
                break;
                Serial.print("BREAK ");
              }
            if ( p >= 0 ){
              p = p-d2;
              x_hasil--;
              y_hasil = y_hasil + 40;
              // Serial.print("x_hasil : ");
              // Serial.println(x_hasil);
              // Serial.print("nilai : ");
              // Serial.println(nilai);
              
            }
            else if(p < 0){
              p = p+d1;
              x_hasil--;
              // y_hasil = y_hasil + 40;
              // Serial.print("x_hasil : ");
              // Serial.println(x_hasil);
              // Serial.print("nilai : ");
              // Serial.println(nilai);
              
            }
            nilai = x_hasil+y_hasil;
          }

        }
  }

  Serial.println("");
  for (int i = 0; i < TOTAL_LENGTH; i++) {
    if (i % 40 == 0 && i > 0) {
      Serial.println();
    }
    Serial.print(outputArray[i], HEX);
    Serial.print(" ");
    
  }
  printer.printBitmap(320, 20, outputArray);
  }
}

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



void touchscreen_read(lv_indev_t *indev, lv_indev_data_t *data) {
    uint16_t x, y;
    bool touched = tft.getTouch(&x, &y);

    if (touched) {
        Serial.printf("x: %d, y: %d\n", x, y);
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = map(x, 0, 480, 0, TFT_HOR_RES);
        data->point.y = map(y, 0, 320, 0, TFT_VER_RES);
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void *draw_buf;

static void anim_x_cb(void *var, int32_t v) {
    lv_obj_set_x((lv_obj_t *)var, v);
}


static void animate_pages(lv_obj_t *current_page, lv_obj_t *next_page) {
    // Hide the current page
    lv_obj_add_flag(current_page, LV_OBJ_FLAG_HIDDEN);

    // Set the next page to be visible
    lv_obj_clear_flag(next_page, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_x(next_page, 0);  // Position it in the default view (no animation)
}

void button_stop_event_cb(lv_event_t *e) {
    // lv_obj_t *btn1 =(lv_obj_t *) lv_event_get_target(e); // Get the button that triggered the event
    // lv_obj_t *btn2 = (lv_obj_t *) lv_event_get_user_data(e); // Get the hidden button (user data)

    // lv_obj_add_flag(btn1, LV_OBJ_FLAG_HIDDEN); // Hide Button1
    // lv_obj_clear_flag(btn2, LV_OBJ_FLAG_HIDDEN); // Show Button2

    lv_obj_add_flag(btn_stop, LV_OBJ_FLAG_HIDDEN);
    // Show Button2, Button3, and Button4
    lv_obj_clear_flag(btn1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(btn2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(btn3, LV_OBJ_FLAG_HIDDEN);
    
    baca_lora = LOW;
    Serial.print("BACA lora ; ");
    Serial.println(baca_lora);
    lv_timer_pause(dataflow_timer);
    lv_timer_pause(datavol_timer);
}



void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
    printer.begin();
    Serial.print("BACA lora ; ");
    Serial.println(baca_lora);
    Serial.printf("Hello Arduino! LVGL %d.%d.%d\n", lv_version_major(), lv_version_minor(), lv_version_patch());
    pinMode(potPin, INPUT);
    uint16_t calData[5] = {332, 3607, 385, 3365, 1};
    tft.init();
    tft.setRotation(3);
    tft.setTouch(calData);

    lv_init();

    draw_buf = heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    lv_display_t *disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, DRAW_BUF_SIZE);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchscreen_read);

    ui_home();
    ui_plotter();
    Serial.println("Setup done");
    vLoraSetup();
}

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


void loop() {
    unsigned int tickPeriod = millis() - lastTickMillis;
    lv_tick_inc(tickPeriod);
    lastTickMillis = millis();
    lv_timer_handler();
    delay(5);

    if (millis() - LastTime >= 1000) {
        LastTime = millis();
        update_rectangle_width(battery);
    }

    if(baca_lora == 1){
      vReadLora();
    }
    
    if(toTransmit){
    int state;
    msg = inputString;
    if(msg == "wake,0"){
      state = radio.setSyncWord(wakeSyncWord, 2);
      // Serial.println(radio.getSyncWord(), BIN);
      radio.setBeaconMode(true);
      Serial.println("wake");
      byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                        0x89, 0xAB, 0xCD, 0xEF};
      state = radio.transmit(byteArr, 8);
    }
    else {
      state = radio.setSyncWord(defaultSyncWord, 2);
      // Serial.println(radio.getSyncWord(), BIN);
      radio.setBeaconMode(false);
      Serial.println("sending");
      state = radio.transmit(msg);
    }

    


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
  if(nonBlockingDelay(3000) && continueSendFlag){
      int state = radio.transmit(msg);
    

    // byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
    //                   0x89, 0xAB, 0xCD, 0xEF};
    // int state = radio.transmit(byteArr, 8);

    if(msg == "wake,0"){
      Serial.println("send wake again");
      byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                        0x89, 0xAB, 0xCD, 0xEF};
      state = radio.transmit(byteArr, 8);
    }
    else {
      state = radio.transmit(msg);
    }

    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(msg + " sent again");
    } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
      Serial.println(F("[SX1278] Packet too long!"));
    } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
      Serial.println(F("[SX1278] Timed out while transmitting!"));
      // Serial.println(radio.getIrqFlags1(), BIN);
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
      
      if(reply == "w" || reply == "flushed!" || reply == "stopped!" || reply == "flushing!"|| reply == "measuring!" || reply == "slept!"){
        if (bConnecting){
          lv_subject_set_int(&fw_update_status_subject, FW_UPDATE_STATE_START_READY);
        }
        continueSendFlag = false;
        radio.setBeaconMode(false);
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

void ui_home() {
    static lv_style_t style_base;
    lv_style_init(&style_base);
    lv_style_set_border_width(&style_base, 0);

    screen = lv_obj_create(lv_screen_active());
    lv_obj_set_size(screen, TFT_HOR_RES, TFT_VER_RES);
    lv_obj_center(screen);
    lv_obj_add_style(screen, &style_base, LV_PART_MAIN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *user_label = lv_label_create(screen);
    lv_label_set_text(user_label, "Uroflow");
    lv_obj_set_style_text_color(user_label, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_text_font(user_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(user_label, LV_ALIGN_TOP_MID, 0, 20);

    static lv_style_t style_btn;
    lv_style_init(&style_btn);
    lv_style_set_radius(&style_btn, 3);
    lv_style_set_bg_color(&style_btn, lv_palette_main(LV_PALETTE_GREEN));

    LV_IMAGE_DECLARE(bat0v2);
    batts = lv_image_create(screen);
    lv_image_set_src(batts, &bat0v2);
    lv_obj_align(batts, LV_ALIGN_BOTTOM_MID, 0, 0);

    scaling_rect = lv_obj_create(screen);
    lv_obj_set_size(scaling_rect, 100, 40);
    lv_obj_set_style_bg_color(scaling_rect, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_align_to(scaling_rect, batts, LV_ALIGN_LEFT_MID, 6, 0);
    lv_obj_clear_flag(scaling_rect, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn = lv_button_create(screen);
    lv_obj_set_size(btn, 100, 50);
    lv_obj_add_style(btn, &style_btn, 0);
    lv_obj_center(btn);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Start");
    lv_obj_center(label);

    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
}

void ui_plotter() {
  screen_plotter = lv_obj_create(lv_scr_act());
  lv_obj_set_size(screen_plotter, TFT_HOR_RES, TFT_VER_RES);
  lv_obj_clear_flag(screen_plotter, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(screen_plotter, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *user_label = lv_label_create(screen_plotter);
  lv_label_set_text(user_label, "Plotter Uroflow");
  lv_obj_set_style_text_color(user_label, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_text_font(user_label, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_align(user_label, LV_ALIGN_TOP_MID, 0, -10);
  
  // static uint16_t point_count = 10; // Initial point count

  chart1 = lv_chart_create(screen_plotter);
  lv_obj_set_size(chart1, 200, 150);
  lv_obj_align(chart1, LV_ALIGN_LEFT_MID, 0, 0);
  lv_chart_set_update_mode(chart1, LV_CHART_UPDATE_MODE_CIRCULAR);
  lv_obj_set_style_size(chart1, 0, 0, LV_PART_INDICATOR);
  lv_chart_set_point_count(chart1, 50);
  
   

  lv_obj_t *chart1_label = lv_label_create(screen_plotter);
  lv_label_set_text(chart1_label, "Flow Rate");
  lv_obj_set_style_text_color(chart1_label, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_text_font(chart1_label, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_align_to(chart1_label, chart1, LV_ALIGN_OUT_TOP_MID, 0, 0);

  chart2 = lv_chart_create(screen_plotter);
  lv_obj_set_size(chart2, 200, 150);
  lv_obj_align(chart2, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_chart_set_update_mode(chart2, LV_CHART_UPDATE_MODE_CIRCULAR);
  lv_obj_set_style_size(chart2, 0, 0, LV_PART_INDICATOR);
  lv_chart_set_point_count(chart2, 50);

  // ser2 = lv_chart_add_series(chart2, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);

  lv_obj_t *chart2_label = lv_label_create(screen_plotter);
  lv_label_set_text(chart2_label, "Volume");
  lv_obj_set_style_text_color(chart2_label, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_text_font(chart2_label, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_align_to(chart2_label, chart2, LV_ALIGN_OUT_TOP_MID, 0, 0);

  dataflow_timer = lv_timer_create(add_dataFlow, 1000, chart1);
  datavol_timer = lv_timer_create(add_dataVol, 1000, chart2);

  static lv_style_t style_btn;
  lv_style_init(&style_btn);
  lv_style_set_radius(&style_btn, 3);
  lv_style_set_bg_color(&style_btn, lv_palette_main(LV_PALETTE_GREEN));

  static lv_style_t style_btn_stop;
  lv_style_init(&style_btn_stop);
  lv_style_set_radius(&style_btn_stop, 3);
  lv_style_set_bg_color(&style_btn_stop, lv_palette_main(LV_PALETTE_RED));

  btn1 = lv_button_create(screen_plotter);
  lv_obj_set_size(btn1, 100, 50);
  lv_obj_add_style(btn1, &style_btn, 0);
  lv_obj_align(btn1, LV_ALIGN_BOTTOM_MID, -150, 0);
  lv_obj_add_flag(btn1, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t * label_btn1 = lv_label_create(btn1);
  lv_label_set_text(label_btn1, "Print");
  lv_obj_center(label_btn1);
  lv_obj_add_event_cb(btn1, event_btn_print, LV_EVENT_CLICKED, NULL);

  btn2 = lv_button_create(screen_plotter);
  lv_obj_set_size(btn2, 100, 50);
  lv_obj_add_style(btn2, &style_btn, 0);
  lv_obj_align(btn2, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_add_event_cb(btn2, event_cb_btn, LV_EVENT_CLICKED, NULL);
  lv_obj_add_flag(btn2, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t * label_btn2 = lv_label_create(btn2);
  lv_label_set_text(label_btn2, "Finish");
  lv_obj_center(label_btn2);
  
  lv_subject_init_int(&fw_update_status_subject, FW_UPDATE_STATE_IDLE);
  lv_subject_add_observer(&fw_update_status_subject, fw_upload_manager_observer_cb, NULL);

  btn3 = lv_button_create(screen_plotter);
  lv_obj_set_size(btn3, 100, 50);
  lv_obj_add_style(btn3, &style_btn, 0);
  lv_obj_align(btn3, LV_ALIGN_BOTTOM_MID, 150, 0);
  lv_obj_add_event_cb(btn3, event_cb_btn2, LV_EVENT_CLICKED, NULL);
  lv_obj_add_flag(btn3, LV_OBJ_FLAG_HIDDEN);


  lv_obj_t * label_btn3 = lv_label_create(btn3);
  lv_label_set_text(label_btn3, "Flush");
  lv_obj_center(label_btn3);

  btn_stop = lv_button_create(screen_plotter);
  lv_obj_set_size(btn_stop, 100, 50);
  lv_obj_add_style(btn_stop, &style_btn_stop, 0);
  lv_obj_align(btn_stop, LV_ALIGN_BOTTOM_MID, 0, 0);

  lv_obj_add_event_cb(btn_stop, button_stop_event_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t * label_btn_stop = lv_label_create(btn_stop);
  lv_label_set_text(label_btn_stop, "Stop");
  lv_obj_center(label_btn_stop);
}

void update_chart_point_count(lv_obj_t *chart, lv_chart_series_t *series, uint16_t *point_count) {
    // Increment the point count
    (*point_count)++;

    // Update the chart's point count
    lv_chart_set_point_count(chart, *point_count);

    // Refresh the chart to reflect the changes
    lv_chart_refresh(chart);
}

static void switch_to_shift_mode(lv_obj_t *chart, bool *is_shift_mode) {
    uint16_t point_count = lv_chart_get_point_count(chart);

    // Backup existing data
    lv_chart_series_t *ser = lv_chart_get_series_next(chart, NULL);
    int16_t temp_data[point_count];
    memcpy(temp_data, ser->y_points, sizeof(int16_t) * point_count);

    // Switch to shift mode
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);

    // Restore the backed-up data
    memcpy(ser->y_points, temp_data, sizeof(int16_t) * point_count);

    lv_chart_refresh(chart);

    // Update the tracking variable
    *is_shift_mode = true;
}



static void add_dataFlow(lv_timer_t *timer) {
    lv_obj_t *chart = (lv_obj_t *)timer->user_data;
    lv_chart_series_t *ser = lv_chart_get_series_next(chart, NULL);
    // int value = flowRate;
    // if (bPage_plotter) {
    //     lv_chart_set_next_value(chart, ser, flowRate);
    // }
    if (bPage_plotter) {
        if (flow_data_count < lv_chart_get_point_count(chart)) {
            // Circular mode
            lv_chart_set_next_value(chart, ser, flowRate);
            flow_data_count++;
        } else {
            // Switch to shift mode and continue adding
            if (!is_flow_chart_shift_mode) {
                switch_to_shift_mode(chart, &is_flow_chart_shift_mode);
            }
            lv_chart_set_next_value(chart, ser, flowRate);
        }
    }
}

static void add_dataVol(lv_timer_t *timer) {
    lv_obj_t *chart = (lv_obj_t *)timer->user_data;
    lv_chart_series_t *ser = lv_chart_get_series_next(chart, NULL);
    // int value = totalVolume;
    // if (bPage_plotter) {
    //     lv_chart_set_next_value(chart, ser, totalVolume);
    // }
    if (bPage_plotter) {
        if (vol_data_count < lv_chart_get_point_count(chart)) {
            // Circular mode
            lv_chart_set_next_value(chart, ser, totalVolume);
            vol_data_count++;
        } else {
            // Switch to shift mode and continue adding
            if (!is_vol_chart_shift_mode) {
                switch_to_shift_mode(chart, &is_vol_chart_shift_mode);
            }
            lv_chart_set_next_value(chart, ser, totalVolume);
        }
    }


}

static int get_simulated_data() {
    simulated_value = analogRead(potPin);
    // simulated_value = simulated_value*0,0244200244200244;
    Serial.println(simulated_value);
    return simulated_value;
}

static void event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        // LV_UNUSED(e);
        lv_obj_t * win = (lv_obj_t *)lv_win_create(lv_screen_active());
        lv_obj_set_size(win, lv_pct(90), lv_pct(90));
        lv_obj_set_height(lv_win_get_header(win), 40);
        lv_obj_set_style_radius(win, 8, 0);
        lv_obj_set_style_shadow_width(win, 24, 0);
        lv_obj_set_style_shadow_offset_x(win, 2, 0);
        lv_obj_set_style_shadow_offset_y(win, 3, 0);
        lv_obj_set_style_shadow_color(win, lv_color_hex3(0x888), 0);
        lv_win_add_title(win, " ");
        lv_obj_t * btn_close = lv_win_add_button(win, LV_SYMBOL_CLOSE, 40);
        lv_obj_add_event_cb(btn_close, fw_update_close_event_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_center(win);

        // String msg = String(0) + "," + String(0) + "," + String(0) + "," + String("w"); // Last value is fixed to 0
        // int state;
        // state = radio.transmit(msg);
        toTransmit = true;
        inputString  = "wake,0";

        lv_subject_set_int(&fw_update_status_subject, FW_UPDATE_STATE_START_CONNECTING);
        lv_subject_add_observer_obj(&fw_update_status_subject, fw_update_win_observer_cb, win, NULL);

        // animate_pages(screen, screen_plotter);
        // bPage_plotter = true;

        
        
        // lv_timer_resume(dataflow_timer);
        // lv_timer_resume(datavol_timer);

    }
}


// static void event_cb(lv_event_t *e) {
//   if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
//     animate_pages(screen, screen_plotter);
//     bPage_plotter = true;
//   }
// }


static void event_cb_btn2(lv_event_t *e) {
    LV_UNUSED(e);
    lv_obj_t * win = (lv_obj_t *)lv_win_create(lv_screen_active());
    lv_obj_set_size(win, lv_pct(90), lv_pct(90));
    lv_obj_set_height(lv_win_get_header(win), 40);
    lv_obj_set_style_radius(win, 8, 0);
    lv_obj_set_style_shadow_width(win, 24, 0);
    lv_obj_set_style_shadow_offset_x(win, 2, 0);
    lv_obj_set_style_shadow_offset_y(win, 3, 0);
    lv_obj_set_style_shadow_color(win, lv_color_hex3(0x888), 0);
    lv_win_add_title(win, "Flush system");
    lv_obj_t * btn = lv_win_add_button(win, LV_SYMBOL_CLOSE, 40);
    lv_obj_add_event_cb(btn, fw_update_close_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_center(win);

    lv_subject_set_int(&fw_update_status_subject, FW_UPDATE_STATE_IDLE);
    lv_subject_add_observer_obj(&fw_update_status_subject, fw_update_win_observer_cb, win, NULL);
}

static void fw_update_close_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_subject_set_int(&fw_update_status_subject, FW_UPDATE_STATE_CANCEL);
}


static void fw_update_win_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * win = (lv_obj_t *)lv_observer_get_target(observer);
    lv_obj_t * cont = lv_win_get_content(win);
    fw_update_state_t status = static_cast<fw_update_state_t>(lv_subject_get_int(&fw_update_status_subject));
    Serial.print("status :");
    Serial.println(status);
    if(status == FW_UPDATE_STATE_IDLE) {
        lv_obj_clean(cont);
        lv_obj_t * spinner = lv_spinner_create(cont);
        lv_obj_center(spinner);
        lv_obj_set_size(spinner, 130, 130);

        lv_obj_t * label = lv_label_create(cont);
        lv_label_set_text(label, "Flushing");
        lv_obj_center(label);

        lv_subject_set_int(subject, FW_UPDATE_STATE_CONNECTING);
    }
    else if(status == FW_UPDATE_STATE_START_CONNECTING) {
        lv_obj_clean(cont);
        lv_obj_t * spinner = lv_spinner_create(cont);
        lv_obj_center(spinner);
        lv_obj_set_size(spinner, 130, 130);

        lv_obj_t * label = lv_label_create(cont);
        lv_label_set_text(label, "Connecting");
        ser1 = lv_chart_add_series(chart1, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);
        ser2 = lv_chart_add_series(chart2, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
        lv_obj_center(label);

        lv_subject_set_int(subject, FW_UPDATE_STATE_S_CONNECTING);
    }
    else if(status == FW_UPDATE_STATE_READY) {
        lv_obj_clean(cont);
        lv_obj_t * label = lv_label_create(cont);
        lv_label_set_text(label, "Finish Flushing");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, -20);

        // animate_pages(screen_plotter, screen);
        // bPage_plotter = false;
        // lv_obj_delete(win);
    }

    else if(status == FW_UPDATE_STATE_START_READY) {
        lv_obj_clean(cont);
        lv_obj_t * label = lv_label_create(cont);
        lv_label_set_text(label, "Device Ready");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, -20);

        lv_subject_set_int(subject, FW_UPDATE_STATE_START_READY_D);

        // animate_pages(screen, screen_plotter);
        // bPage_plotter = true;
        // lv_obj_delete(win);
    }
    
    else if(status == FW_UPDATE_STATE_START_FINISH) {
        lv_obj_clean(cont);
        lv_obj_t * label = lv_label_create(cont);
        lv_label_set_text(label, "Device Ready");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, -20);
        
        animate_pages(screen, screen_plotter);
        Serial.println("Titik nol");
        titik = 0;

        lv_chart_set_update_mode(chart1, LV_CHART_UPDATE_MODE_CIRCULAR);
        lv_chart_set_update_mode(chart2, LV_CHART_UPDATE_MODE_CIRCULAR);

        reset_chart_series(chart1); // Remove all existing series
        ser1 = lv_chart_add_series(chart1, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y); // Re-add series
        
        // Reset series in chart2
        reset_chart_series(chart2); // Remove all existing series
        ser2 = lv_chart_add_series(chart2, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y); // Re-add series

        // Optionally log reset
        Serial.println("Charts reset.");
        
        lv_timer_resume(dataflow_timer);
        lv_timer_resume(datavol_timer);
        bPage_plotter = true;
        lv_obj_delete(win);
    }

    else if(status == FW_UPDATE_STATE_STOP) {
        Serial.println("stop clicked");
    }

    else if(status == FW_UPDATE_STATE_CANCEL) {
        lv_obj_delete(win);
    }
}

static void connect_timer_cb(lv_timer_t * t)
{
    if(lv_subject_get_int(&fw_update_status_subject) != FW_UPDATE_STATE_CANCEL) {
        lv_subject_set_int(&fw_update_status_subject, FW_UPDATE_STATE_READY);
    }
    lv_timer_delete(t);
}

static void connect_timer_cb2(lv_timer_t * t)
{
    if(lv_subject_get_int(&fw_update_status_subject) != FW_UPDATE_STATE_CANCEL) {
        lv_subject_set_int(&fw_update_status_subject, FW_UPDATE_STATE_START_FINISH);
    }
    lv_timer_delete(t);
}

static void connect_timer_s_connected2(lv_timer_t * t)
{ 
    if(lv_subject_get_int(&fw_update_status_subject) != FW_UPDATE_STATE_CANCEL) {
        lv_subject_set_int(&fw_update_status_subject, FW_UPDATE_STATE_START_READY);
    }
    lv_timer_delete(t);
}

static void download_timer_cb(lv_timer_t * t)
{
    if(lv_subject_get_int(&fw_update_status_subject) == FW_UPDATE_STATE_CANCEL) {
        lv_timer_delete(t);
        return;
    }

    int32_t v = lv_subject_get_int(&fw_download_percent_subject);
    if(v < 100) {
        lv_subject_set_int(&fw_download_percent_subject, v + 1);
    }
    else {
        lv_subject_set_int(&fw_update_status_subject, FW_UPDATE_STATE_READY);
        lv_timer_delete(t);
    }
}

static void fw_upload_manager_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    LV_UNUSED(subject);
    LV_UNUSED(observer);

    fw_update_state_t state = static_cast<fw_update_state_t>(lv_subject_get_int(&fw_update_status_subject));
    if(state == FW_UPDATE_STATE_CONNECTING) {
        lv_timer_create(connect_timer_cb, 5000, NULL);
    }
    else if(state == FW_UPDATE_STATE_CONNECTED) {
        lv_subject_set_int(&fw_download_percent_subject, 0);
        lv_subject_set_int(&fw_update_status_subject, FW_UPDATE_STATE_DOWNLOADING);
        lv_timer_create(download_timer_cb, 50, NULL);
    }
    else if(state == FW_UPDATE_STATE_S_CONNECTING) {
        
      bConnecting = true;
    }
    else if(state == FW_UPDATE_STATE_START_READY_D) {
        lv_timer_create(connect_timer_cb2, 2000, NULL);
        bConnecting = false;
    }
}

static void event_cb_btn(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    // // Reset series in chart1
    // reset_chart_series(chart1); // Remove all existing series
    // ser1 = lv_chart_add_series(chart1, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y); // Re-add series
    
    // // Reset series in chart2
    // reset_chart_series(chart2); // Remove all existing series
    // ser2 = lv_chart_add_series(chart2, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y); // Re-add series

    // // Optionally log reset
    // Serial.println("Charts reset.");
    lv_obj_clear_flag(btn_stop, LV_OBJ_FLAG_HIDDEN);

    // Show Button2, Button3, and Button4
    lv_obj_add_flag(btn1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(btn2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(btn3, LV_OBJ_FLAG_HIDDEN);

    // Handle any other logic like page animations or status changes
    animate_pages(screen_plotter, screen); // Example animation back to home screen
    baca_lora = HIGH;

  }
}

static void event_btn_print(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    Serial.print("Flow :");
    int inputData[] = {0,0,0,0}; // Contoh input array dengan 40 sebagai trigger
    int dataSize = sizeof(inputData) / sizeof(inputData[0]);
      for (int i = 0; i < titik; i++) {
    // if (i % 40 == 0 && i > 0) {
    //   Serial.println();
    // }
    Serial.print(array_flow[i]);
    Serial.print(" ");
      }
    convertArray(array_flow, titik, outputArray);
    Serial.println(" ");
    Serial.print("Volume :");
    for (int i = 0; i < titik; i++) {
    // if (i % 40 == 0 && i > 0) {
    //   Serial.println();
    // }
    Serial.print(array_volum[i]);
    Serial.print(" ");
  }
  convertArray(inputData, 4, outputArray);

  convertArray(array_volum, titik, outputArray);
    

  }   

}

// Function to reset the chart series
static void reset_chart_series(lv_obj_t *chart) {
    lv_chart_series_t *ser = lv_chart_get_series_next(chart, NULL); // Get the first series
    while (ser) {
        lv_chart_remove_series(chart, ser); // Remove the current series
        ser = lv_chart_get_series_next(chart, NULL); // Get the next series
}
}