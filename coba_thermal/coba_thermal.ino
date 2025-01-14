
// // Helper function to format a number with leading spaces
// String padStart(int number, int width) {
//   String result = String(number);
//   while (result.length() < width) {
//     result = ' ' + result;
//   }
//   return result;
// }

// void setup() {
//   Serial2.begin(115200); // Set baud rate for your printer
//   delay(1000); // Wait for the printer to initialize

//   // Clear and reset printer
//   Serial2.write(0x1B);
//   Serial2.write('@'); // Initialize printer

//   // Enable page mode
//   Serial2.write(0x1B);
//   Serial2.write('L'); // Enter page mode
//   Serial2.write(0x1B); 
//   Serial2.write('T'); // Set top margin
//   Serial2.write(0x00); // No top margin

//   // Print graph title
//   Serial2.println("High-Density Graph Example");
//   Serial2.println("----------------------------");

//   // Create a buffer for the graph to store the X and Y positions of the stars
//   char graph[10][50]; // 10 rows (Y-axis) and 50 columns (X-axis)

//   // Initialize the graph with spaces
//   for (int i = 0; i < 10; i++) {
//     for (int j = 0; j < 50; j++) {
//       graph[i][j] = ' ';
//     }
//   }

//   // Create a line by plotting stars and connecting them with lines
//   for (int i = 0; i < 50; i++) {
//     int y = (i % 10);  // Example of a pattern for the y-axis value
//     graph[y][i] = '*'; // Plot point at (y, i)
//     if (i > 0) {
//       // Connect previous point with a line
//       int prevY = (i - 1) % 10;
//       if (graph[prevY][i - 1] == '*') {
//         graph[y][i - 1] = '*'; // Draw a line connecting the points
//       }
//     }
//   }

//   // Print Y-axis labels and graph
//   for (int i = 9; i >= 0; i--) { // Y-axis steps from top (100) to bottom (0)
//     char label[4];
//     sprintf(label, "%3d|", i * 10); // Scale Y-axis from 0 to 100
//     Serial2.print(label);
    
//     // Print each row of the graph with stars connected by a line
//     for (int j = 0; j < 50; j++) {
//       Serial2.print(graph[i][j]);
//     }
//     Serial2.println();
//   }

//   // Print X-axis
//   Serial2.print("    +");
//   for (int k = 0; k < 50; k++) {
//     Serial2.print('-');
//   }
//   Serial2.println(">");

//   // Print X-axis labels
//   Serial2.print("     ");
//   for (int l = 0; l <= 100; l += 10) {
//     Serial2.print(padStart(l, 5));
//   }
//   Serial2.println();

//   Serial2.println("Graph complete!");

//   // End page mode and print the page
//   Serial2.write(0x1B); // ESC
//   Serial2.write('J');  // Print and resume normal mode (form feed)
//   Serial2.write(0x00); // No additional lines to skip
//   delay(1000); // Allow time for printing
// }

// void loop() {
//   // No continuous loop functionality needed
// }


#include "Adafruit_Thermal.h"


// Create the printer object
Adafruit_Thermal printer(&Serial2);

void setup() {
  Serial2.begin(115200); // Set baud rate to match your printer's default
  printer.begin();

  // printer.setHeatConfig(255,255,255); 
  // printer.setPrintDensity(15, 30); // Adjust these values as needed

  // Test print for thickness/density
  printer.print("N");
  printer.print("a");

  printer.feed(3);
}

void loop() {
  // Do nothing
}


