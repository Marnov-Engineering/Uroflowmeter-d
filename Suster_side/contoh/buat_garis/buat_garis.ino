char SendStr[16];
int data = 1;
void setup() {
  Serial2.begin(115200);  // Initialize Serial2 with 115200 baud rate

  // Initialize printer
  Serial2.write(0x1B);  // ESC
  Serial2.write(0x40);  // @ (Initialize command)

  // Enable wide text mode (ESC 0x21 0x20)
  for (int i = 0; i < 5; i++) {
  SendStr[0]=0x1D;
  SendStr[1]=0x27;
  SendStr[2]=1; // A line of
  SendStr[3]=0;
  SendStr[4]=0; //The starting point
  SendStr[5]=120;
  SendStr[6]=1; //The end point
  PreSendData(SendStr, 7);  // Send the wide text command
  }

  Serial2.write(0x1B);  // ESC
  Serial2.write(0x40);  // @ (Initialize command)
  // 0x1B, 0x26, 0x03, 0x40, 0x40, 0x0C, 0x00, 0x00, 0x06, 0x00, 0x00, 0x04, 0x00, 0x00, 0x1C
  SendStr[0]=0x1B;
  SendStr[1]=0x2A;
  SendStr[2]=0x01; //Three lines:X-axis,sin and cos function curve
  SendStr[3]=0x0; 
  SendStr[4]=0x02; // The X axis location
  SendStr[5]=0x0C; 
  SendStr[6]=0x00;
  SendStr[7]=0x00;
  SendStr[8]=0x06;
  SendStr[9]=0x00;
  SendStr[10]=0x00;
  SendStr[11]=0x04;
  SendStr[12]=0x00;
  SendStr[13]=0x00;
  SendStr[14]=0x1C;
  PreSendData(SendStr, 15);  // Send the wide text command

  // Print wide text
  Serial2.println("This is Wide");

  // Reset to normal text mode (ESC 0x21 0x00)
  // SendStr[2] = 0x00;  // 0x00 resets to normal text
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
