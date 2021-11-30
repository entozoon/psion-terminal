#ifndef BOARD_HAS_PSRAM
#error "Please enable PSRAM !!!"
#endif
#include <Arduino.h>
// Happily includes from lib/package/src/ if necessary
#include "FiraMono.h"
#include "epd_driver.h"
#define BATT_PIN 36
int x;
int y;
uint8_t *framebuffer;
void drawAreaFromFramebuffer(Rect_t area = epd_full_screen(),
                             uint8_t *framebuffer = framebuffer) {
  epd_draw_image(area, framebuffer, BLACK_ON_WHITE);
}
void writeYeTerminal(const GFXfont *font, const char *string, int *cursor_x,
                     int *cursor_y, uint8_t *framebuffer, double lineHeight = 1,
                     bool vertical = false) {
  // Nicked from epd_driver.h write_string but using write_mode for white text,
  // line height, bottom-up layout, etc
  char *token, *newstring, *tofree;
  if (string == NULL || newstring == NULL) {
    return;
  }
  tofree = newstring = strdup(string);
  // taken from the strsep manpage
  int line_start = *cursor_x;
  while ((token = strsep(&newstring, "\n")) != NULL) {
    *cursor_x = line_start;
    write_mode(font, token, cursor_x, cursor_y, framebuffer, WHITE_ON_WHITE,
               NULL);
    *cursor_y += font->advance_y * lineHeight;
  }
  free(tofree);
}
void setup() {
  Serial.begin(115200);
  // epd_set_rotation(2);
  framebuffer =
      (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
  if (!framebuffer) {
    Serial.println("alloc memory failed !!!");
    while (1)
      ;
  }
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
  epd_init();
  epd_poweron();
  epd_clear();
  epd_fill_rect(0, 0, EPD_WIDTH, EPD_HEIGHT, 0, framebuffer);
  epd_draw_rect(600, 450, 120, 60, 0, framebuffer);
  epd_fill_circle(200, 200, 200, 230, framebuffer);
  epd_fill_circle(300, 300, 200, 210, framebuffer);
  epd_fill_circle(400, 400, 200, 180, framebuffer);
  epd_fill_circle(500, 500, 200, 130, framebuffer);
  epd_fill_circle(600, 600, 200, 0, framebuffer);
  //
  // drawAreaFromFramebuffer();
  //
  //
  // writeYeTerminal into buffer is broken for white text. I'd have to:
  // - fuck about with draw_char, passing mode and figuring the pixel colours
  // out,
  // - might as well fix write_string passing mode there to but doesn't look
  // like they're accepting PRs
  // - might also as well add like, a line-height param
  //
  // x = 300;
  // y = 300;
  // writeYeTerminal((GFXfont *)&FiraMono,
  // "Lots\nand\nlots\nof\njuicy\nnew\nlines\nbaby!", &x, &y, NULL, .8, true);
  //
  // AND there's probably a cleverer way of doing this, like
  // Only writing the bottom line and copy paste dumping the framebuffer pixels
  // upward
  //
  // !! In fact, don't bother continuing with vertical=true until I've had a
  // thought about this:: !! Full text buffer:
  // - Draw individual characters along the bottom line
  // - When \n detected, dump it in buffer above
  //
  // Pixels *******:
  // - Draw individual characters along the bottom line
  // - When \n detected, draw entire framebuffer y: -30px onto the screen
  // - Clear bottom line area
  //
  // Draw bottom half to top or something
  // epd_clear();
  //
  epd_fill_rect(0, 0, EPD_WIDTH, EPD_HEIGHT / 2, 0, framebuffer);
  // epd_draw_image(epd_full_screen(), framebuffer, BLACK_ON_WHITE);
  // delay(2000);
  // Single lines
  x = 0;
  y = 480;
  char *string0 = "123456789012345678901234567890123456789012345678901234567890"
                  "12345678901234567890";
  write_mode((GFXfont *)&FiraMono, string0, &x, &y, framebuffer, BLACK_ON_WHITE,
             NULL);
  x = 0;
  y = 500;
  char *string1 = "123456789012345678901234567890123456789012345678901234567890"
                  "12345678901234567890";
  write_mode((GFXfont *)&FiraMono, string1, &x, &y, framebuffer, WHITE_ON_WHITE,
             NULL);
  x = 0;
  y = 530;
  char *string2 = "123456789012345678901234567890123456789012345678901234567890"
                  "12345678901234567890";
  write_mode((GFXfont *)&FiraMono, string2, &x, &y, framebuffer, WHITE_ON_BLACK,
             NULL);
  //
  epd_draw_image(epd_full_screen(), framebuffer, WHITE_ON_BLACK);
  delay(3000);
  //
  // Fill top black
  epd_fill_rect(0, 0, EPD_WIDTH, EPD_HEIGHT / 2, 255, framebuffer);
  delay(1000);
  // Redraw top half only
  drawAreaFromFramebuffer(Rect_t{
      .x = 0,
      .y = 0,
      .width = EPD_WIDTH,
      .height = EPD_HEIGHT / 2,
  });
  delay(3000);
  // Copy screen offset vertically up
  epd_draw_image(
      Rect_t{
          .x = 0,
          .y = -EPD_HEIGHT / 2,
          .width = EPD_WIDTH,
          .height = EPD_HEIGHT,
      },
      framebuffer, BLACK_ON_WHITE);
  delay(3000);
  // Redraw everything
  drawAreaFromFramebuffer();
  epd_poweroff();
  delay(5000);
}
void batteryMeter() {
  uint16_t v = analogRead(BATT_PIN);
  // (From demo code 🤷🏼‍♀️)
  int vref = 1100;
  float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
  String voltage = String(battery_voltage) + "V\nfoo\nbar\nbaz";
  int x = 0;
  int y = 20;
  // int x = EPD_WIDTH - 100;
  // int y = 0;
  epd_clear_area(Rect_t{
      .x = x,
      .y = y,
      .width = 100,
      .height = 50,
  });
  write_mode((GFXfont *)&FiraMono, (char *)voltage.c_str(), &x, &y, NULL,
             WHITE_ON_WHITE, NULL);
}
void loop() {
  // epd_poweron();
  // batteryMeter();
  // epd_poweroff_all();
  // delay(5000);
}