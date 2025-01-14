#define TOTAL_LENGTH 1600
#include "Adafruit_Thermal.h"

int urutan_data = 50;
int data_sensor[] = {29, 32, 10, 27, 26, 15, 0, 0, 38, 27, 2, 30, 38, 17, 23, 35, 14, 37, 28, 36, 12, 32, 12, 24, 23, 33, 31, 6, 15, 6, 23, 39, 14, 36, 3, 32, 31, 13, 12, 33, 19, 13, 14, 12, 38, 30, 32, 8, 36, 38};

int data1;
int data2;


#define TX_PIN 17 // Arduino transmit  YELLOW WIRE  labeled RX on printer
#define RX_PIN 16
Adafruit_Thermal printer(&Serial2); 

uint8_t PROGMEM outputArray[TOTAL_LENGTH];
// int awal = 20;
// int akhir = 30;
int hasil;
int titik = 40;
int pinggir;

int x_1;
int x_2;
int y_1 = 0;
int y_2 = 40;
int x_hasil;
int y_hasil;

int dx;
int dy;
int d1;
int d2;
int p;
int m;
int koordinat;


int stop; // 40 dibagi input
int j = 0;
// int p;
int interval;
int nilai; 
int value = 0;

void convertArray(const int *data, int dataSize, uint8_t *output) {
  // Inisialisasi array output dengan nilai 0x00
  // for (int i = 0; i < TOTAL_LENGTH; i++) {
  //   output[i] = 0x00;
  // }

  for(int K = 0; K < urutan_data;K++){

    for (int i = 0; i < TOTAL_LENGTH; i++) {
    output[i] = 0x00;
  }
  
    x_1 = data_sensor[K];
    if(K == (urutan_data-1) ){
      break;
    }
    else{
      x_2 = data_sensor[K+1];
    }
    
    
    // data2 = data[K+1];
    // Serial.print("data1 : ");
    // Serial.println(x_1);
    // Serial.print("data2 : ");
    // Serial.println(x_2);

  

  if(x_1 < x_2){
    dx = x_2 - x_1;
    dy = y_2 - y_1;
    d1 = 2*dx;
    d2 = 2*(dx-dy);
    p = d1 - dy;

    x_hasil = x_1;
    y_hasil = y_1;
    nilai = (x_1+y_1)-1;

    // Serial.print("Koordinat : ");
    // Serial.println(nilai);
    // Serial.print("dx : ");
    // Serial.println(dx);
    // Serial.print("dy : ");
    // Serial.println(dy);
    // Serial.print("d1 : ");
    // Serial.println(d1);
    // Serial.print("d2 : ");
    // Serial.println(d2);
    // Serial.print("p : ");
    // Serial.println(p);

    for (int i = 1; i <= 40; i++) {
      // Serial.print("Iterasi ke-");
      // Serial.println(i);
      nilai = x_hasil+y_hasil;
      output[nilai] = 0xFF;
      // nilai = nilai+1;
      // output[nilai] = 0xFF;
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
  else{
        dx = x_1 - x_2;
        dy = y_2 - y_1;
        d1 = 2*dx;
        d2 = 2*(dx-dy);
        p = d1 - dy;

        x_hasil = x_1;
        y_hasil = y_1;
        nilai = (x_1+y_1)-1;

        // Serial.print("Koordinat : ");
        // Serial.println(nilai);
        // Serial.print("dx : ");
        // Serial.println(dx);
        // Serial.print("dy : ");
        // Serial.println(dy);
        // Serial.print("d1 : ");
        // Serial.println(d1);
        // Serial.print("d2 : ");
        // Serial.println(d2);
        // Serial.print("p : ");
        // Serial.println(p);

        for (int i = 1; i <= 40; i++) {
          // Serial.print("Iterasi ke-");
          // Serial.println(i);
          nilai = x_hasil+y_hasil;
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
      } 
  }
    Serial.println(" ");
  for (int i = 0; i < TOTAL_LENGTH; i++) {
    if (i % 40 == 0 && i > 0) {
      Serial.println();
    }
    Serial.print(outputArray[i], HEX);
    Serial.print(" ");
  }
  // printer.printBitmap(320, 40, outputArray);
  }
  

  // if ( awal > akhir ){
  //     hasil = awal - akhir;
  //     stop = 40/hasil;
  //     Serial.println("nilai hasil : awalan janA");

  //     for (int i = 1; i <= 40; i++) {
  //       if( j == stop){
  //         p--;
  //         j = 0;
  //       }
  //       Serial.print("Iterasi ke-");
  //       nilai = (((40*i)+p)-40)+(awal-1);
  //       Serial.print(i);
  //       Serial.print("Koordinat : ");
  //       Serial.println(nilai);
  //       // if( i % 40 == 0){
  //       //   output[nilai] = 0xF0;
  //       // }
  //       output[nilai] = 0xFF;
  //       j++;
  //       // delay(1000);  // Delay 1 detik antara setiap iterasi
  //     }


  //   } 
  //   else if ( akhir > awal ){
  //     hasil = akhir - awal;
  //     stop = 40/hasil;
  //     Serial.println("nilai hasil : akhir");

  //       for (int i = 1; i <= 40; i++) {
  //         if( j == stop){
  //           p++;
  //           j = 0;
  //         }
  //         Serial.print("Iterasi ke-");
  //         nilai = (((40*i)+p)-40)+awal;
  //         Serial.print(i);
  //         Serial.print("Koordinat : ");
  //         Serial.println(nilai);
  //       //   if( i % 40 == 0){
  //       //   output[nilai] = 0xF0;
  //       // }
  //         output[nilai] = 0xFF;
  //         j++;
  //         // delay(1000);  // Delay 1 detik antara setiap iterasi
  //       }


  //   }
  //   else if(akhir == awal){
  //     Serial.println("nilai hasil : sama");
  //   }
    
    // Serial.print("nilai hasil : ");
    // Serial.println(hasil);
    // Serial.print("nilai stop : ");
    // Serial.println(stop);


  // for (int i = 1; i <= 40; i++) {
  //   if( j == stop){
  //     p++;
  //     j = 0;
  //   }
  //   Serial.print("Iterasi ke-");
  //   nilai = ((40*i)+p)-40;
  //   Serial.print(i);
  //   Serial.print("Koordinat : ");
  //   Serial.println(nilai);
  //   output[nilai] = 0xFF;
  //   j++;
  //   // delay(1000);  // Delay 1 detik antara setiap iterasi
  // }

}

void setup() {
  Serial.begin(115200);
  // Serial.println("a");
  // data[0]= 0;
  // data[1]= 10;
  // data[2]= 30;
  // data[3]= 20;
  // data[4]= 40;
  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  // Serial.println("b");
  printer.begin();
  int inputData[] = {40}; // Contoh input array dengan 40 sebagai trigger
  int dataSize = sizeof(inputData) / sizeof(inputData[0]);

   outputArray[TOTAL_LENGTH]; // Array output

  // Panggil fungsi untuk konversi array
  convertArray(inputData, dataSize, outputArray);

  // Cetak hasil konversi
  // Serial.println("Output Array:");
  // for (int i = 0; i < TOTAL_LENGTH; i++) {
  //   if (i % 40 == 0 && i > 0) {
  //     Serial.println();
  //   }
  //   Serial.print(outputArray[i], HEX);
  //   Serial.print(" ");
  // }
  // printer.printBitmap(320, 40, outputArray);
}

void loop() {
  // Tidak ada kode di loop
}
