#include <Arduino.h>
// Basic bitch debug *********
#include "SoftwareSerial.h"
#define RX 15
#define TX 14 // unused
SoftwareSerial softwareSerial;
uint8_t escMode = 0;
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nLet's go!");
  // Pi: 115200 baud, 8 bits, no parity, 1 stop bit
  softwareSerial.begin(115200, SWSERIAL_8N1, RX, TX, false);
  softwareSerial.enableIntTx(false);
  if (!softwareSerial) {
    Serial.println("SoftwareSerial problem");
    while (1) {
      delay(1000);
    }
  }
}
// void checkSwSerial(SoftwareSerial *ss) {
//   if (ss->available()) {
//     while (ss->available()) {
//       Serial.print(ss->read());
//       // byte ch = (byte)ss->read();
//       // Serial.print(ch);
//       // Serial.print(ch < 0x10 ? F(" 0") : F(" "));
//       // Serial.print(ch, HEX);
//     }
//     // Serial.println();
//   }
// }
// void loop() { checkSwSerial(&softwareSerial); }
void loop() {
  // NB can add buffer capacity in .begin! (see swsertest)
  // NNB it does have plenty of hardware serial, but no breakout pins although
  // certainly worth a try, bottom left third up
  // I think this is it but.. just wild messy crap. Some dude says
  // softwareSerial is unreliable even below this baud
  String content = "";
  while (softwareSerial.available() > 0) {
    char c = (char)softwareSerial.read();
    content.concat(c);
    // Serial.print((char)softwareSerial.read());
    // int ch = softwareSerial.read();
    // Serial.print((char)ch);
    // yield();
  }
  Serial.print(content);
}