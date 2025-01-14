char SendStr[5];

void setup() {
  Serial2.begin(115200);  // Initialize Serial2 with 115200 baud rate

  // Initialize printer
  Serial2.write(0x1B);  // ESC
  Serial2.write(0x40);  // @ (Initialize command)

  // Enable wide text mode (ESC 0x21 0x20)
  SendStr[0] = 0x1B;  // ESC
  SendStr[1] = 0x21;  // 0x21 sets text formatting
  SendStr[2] = 0x20;  // 0x20 enables wide text (change to 0x01 for bold, etc.)
  PreSendData(SendStr, 3);  // Send the wide text command

  // Print wide text
  Serial2.println("This is Wide");

  // Reset to normal text mode (ESC 0x21 0x00)
  SendStr[2] = 0x00;  // 0x00 resets to normal text
  PreSendData(SendStr, 3);  // Send the reset command

  // Print normal text
  Serial2.println("This is NORMAL");
}

void loop() {
  // No actions required in loop
}

void PreSendData(const char *data, size_t length) {
  // Send raw data to the printer
  Serial2.write((uint8_t *)data, length);
}
