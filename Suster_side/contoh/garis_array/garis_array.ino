#define TOTAL_LENGTH 1600

void convertArray(const int *data, int dataSize, uint8_t *output) {
  // Inisialisasi array output dengan nilai 0x00
  for (int i = 0; i < TOTAL_LENGTH; i++) {
    output[i] = 0x00;
  }

  // Mengisi nilai 0xFF pada posisi tertentu
  for (int i = 0; i < dataSize; i++) {
    if (data[i] == 40) {
      for (int j = 1; j <= TOTAL_LENGTH; j += 41) {
        if (j - 1 < TOTAL_LENGTH) { // Cegah akses di luar batas array
          output[j - 1] = 0xFF; // Indeks dimulai dari 0
        }
      }
    }
  }
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
