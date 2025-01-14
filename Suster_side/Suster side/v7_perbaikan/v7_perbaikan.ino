#include <TFT_eSPI.h>
#include <lvgl.h>
#include "RadioLib.h"

#define TFT_HOR_RES   480
#define TFT_VER_RES   320

#define MY_SCLK  14//18
#define MY_MISO  33//12 //19
#define MY_MOSI  13//13 //23
#define MY_CS    32//32

SPIClass SPI_2(3);
SPISettings spiSettings(2000000, MSBFIRST, SPI_MODE0);
SX1276 radio = new Module(32, 27, 26, 33, SPI_2, spiSettings);
volatile bool receivedFlag = false;

TFT_eSPI tft = TFT_eSPI(); 
int batt = 100;
int uro = 0;
bool bPage_plotter = false;
uint32_t LastTime;
static int simulated_value = 0;
const int potPin = 35;