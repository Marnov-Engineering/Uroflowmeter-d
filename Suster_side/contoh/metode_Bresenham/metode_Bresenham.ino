#define TOTAL_LENGTH 1600
int data[5] = {10, 20, 30, 40, 10};

// int awal = 20;
// int akhir = 30;
int hasil;
int titik = 40;
int pinggir;

int x_1 = 28;
int x_2 = 16;
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

void convertArray(const int *data, int dataSize, uint8_t *output) {
  // Inisialisasi array output dengan nilai 0x00
  for (int i = 0; i < TOTAL_LENGTH; i++) {
    output[i] = 0x00;
  }
  if(x_1 < x_2){
    dx = x_2 - x_1;
    dy = y_2 - y_1;
    d1 = 2*dx;
    d2 = 2*(dx-dy);
    p = d1 - dy;

    x_hasil = x_1;
    y_hasil = y_1;
    nilai = (x_1+y_1)-1;

    Serial.print("Koordinat : ");
    Serial.println(nilai);
    Serial.print("dx : ");
    Serial.println(dx);
    Serial.print("dy : ");
    Serial.println(dy);
    Serial.print("d1 : ");
    Serial.println(d1);
    Serial.print("d2 : ");
    Serial.println(d2);
    Serial.print("p : ");
    Serial.println(p);

    for (int i = 1; i <= 40; i++) {
      Serial.print("Iterasi ke-");
      Serial.println(i);
      nilai = x_hasil+y_hasil;
      output[nilai] = 0xFF;
      j++;
      // y_hasil = y_hasil/40;

      if ( p >= 0 ){
      p = p+d2;
      x_hasil++;
      y_hasil = y_hasil + 40;
      Serial.print("x_hasil : ");
      Serial.println(x_hasil);
      Serial.print("y_hasil : ");
      Serial.println(y_hasil);
    }
    else if(p < 0){
      p = p+d1;
      y_hasil = y_hasil + 40;
      Serial.print("x_hasil : ");
      Serial.println(x_hasil);
      Serial.print("y_hasil : ");
      Serial.println(y_hasil);
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

        Serial.print("Koordinat : ");
        Serial.println(nilai);
        Serial.print("dx : ");
        Serial.println(dx);
        Serial.print("dy : ");
        Serial.println(dy);
        Serial.print("d1 : ");
        Serial.println(d1);
        Serial.print("d2 : ");
        Serial.println(d2);
        Serial.print("p : ");
        Serial.println(p);

        for (int i = 1; i <= 40; i++) {
          Serial.print("Iterasi ke-");
          Serial.println(i);
          nilai = x_hasil+y_hasil;
          output[nilai] = 0xFF;
          j++;
          // y_hasil = y_hasil/40;

          if ( p >= 0 ){
            p = p+d2;
            x_hasil--;
            y_hasil = y_hasil + 40;
            Serial.print("x_hasil : ");
            Serial.println(x_hasil);
            Serial.print("y_hasil : ");
            Serial.println(y_hasil);
          }
        else if(p < 0){
          p = p+d1;
          y_hasil = y_hasil + 40;
          Serial.print("x_hasil : ");
          Serial.println(x_hasil);
          Serial.print("y_hasil : ");
          Serial.println(y_hasil);
        }
      } 
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
    
    Serial.print("nilai hasil : ");
    Serial.println(hasil);
    Serial.print("nilai stop : ");
    Serial.println(stop);


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
  int inputData[] = {40}; // Contoh input array dengan 40 sebagai trigger
  int dataSize = sizeof(inputData) / sizeof(inputData[0]);

  uint8_t outputArray[TOTAL_LENGTH]; // Array output

  // Panggil fungsi untuk konversi array
  convertArray(inputData, dataSize, outputArray);

  // Cetak hasil konversi
  Serial.println("Output Array:");
  for (int i = 0; i < TOTAL_LENGTH; i++) {
    if (i % 40 == 0 && i > 0) {
      Serial.println();
    }
    Serial.print(outputArray[i], HEX);
    Serial.print(" ");
  }
}

void loop() {
  // Tidak ada kode di loop
}
