#include <Arduino.h>
#define TX (GPIO_NUM_14)
#define RX (GPIO_NUM_15)
void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, RX, TX); // !!
  delay(1000);
  Serial.println("\nLet's Go Already!");
}
void loop() {
  if (Serial1.available() > 0) {
    char c = Serial1.read();
    if (c == 13) {
      Serial.println("");
    } else {
      Serial.print(c);
    }
  }
}