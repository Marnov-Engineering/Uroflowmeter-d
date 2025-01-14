#define TOTAL_LENGTH 800
int data[5] = {10, 20, 30, 40, 10};

int awal = 30;
int akhir = 20;
int hasil;
int titik = 40;
int pinggir;

int stop; // 40 dibagi input
int j = 0;
int p;
int interval;
int nilai; 

void convertArray(const int *data, int dataSize, uint8_t *output) {
  // Inisialisasi array output dengan nilai 0x00
  for (int i = 0; i < TOTAL_LENGTH; i++) {
    output[i] = 0x00;
  }

  if ( awal > akhir ){
      hasil = awal - akhir;
      stop = 40/hasil;
      Serial.println("nilai hasil : awalan janA");

      for (int i = 1; i <= 40; i++) {
        if( j == stop){
          p--;
          j = 0;
        }
        Serial.print("Iterasi ke-");
        nilai = (((40*i)+p)-40)+(awal-1);
        Serial.print(i);
        Serial.print("Koordinat : ");
        Serial.println(nilai);
        // if( i % 40 == 0){
        //   output[nilai] = 0xF0;
        // }
        output[nilai] = 0xFF;
        j++;
        // delay(1000);  // Delay 1 detik antara setiap iterasi
      }


    } 
    else if ( akhir > awal ){
      hasil = akhir - awal;
      stop = 40/hasil;
      Serial.println("nilai hasil : akhir");

        for (int i = 1; i <= 40; i++) {
          if( j == stop){
            p++;
            j = 0;
          }
          Serial.print("Iterasi ke-");
          nilai = (((40*i)+p)-40)+awal;
          Serial.print(i);
          Serial.print("Koordinat : ");
          Serial.println(nilai);
        //   if( i % 40 == 0){
        //   output[nilai] = 0xF0;
        // }
          output[nilai] = 0xFF;
          j++;
          // delay(1000);  // Delay 1 detik antara setiap iterasi
        }


    }
    else if(akhir == awal){
      Serial.println("nilai hasil : sama");
    }
    
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
