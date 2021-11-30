#include <Arduino.h>
// esp32 sdk imports
#include "esp_heap_caps.h"
#include "esp_log.h"
// epd
#include "epd_driver.h"
#include "epd_highlevel.h"
// battery
#include "esp_adc_cal.h"
#include <driver/adc.h>
// deepsleep
#include "esp_sleep.h"
// font
#include "FiraMono.h"
#define BATT_PIN 36
#define WAVEFORM EPD_BUILTIN_WAVEFORM
/**
 * Upper most button on side of device. Used to setup as wakeup source to start
 * from deepsleep. Pinout here
 * https://ae01.alicdn.com/kf/H133488d889bd4dd4942fbc1415e0deb1j.jpg
 */
gpio_num_t FIRST_BTN_PIN = GPIO_NUM_39;
EpdiyHighlevelState hl;
// ambient temperature around device
int temperature = 20;
uint8_t *fb;
enum EpdDrawError err;
// CHOOSE HERE YOU IF YOU WANT PORTRAIT OR LANDSCAPE
// both orientations possible
EpdRotation orientation = EPD_ROT_INVERTED_LANDSCAPE;
// EpdRotation orientation = EPD_ROT_LANDSCAPE;
int vref = 1100;
/**
 * RTC Memory var to get number of wakeups via wakeup source button
 * For demo purposes of rtc data attr
 **/
RTC_DATA_ATTR int pressed_wakeup_btn_index;
/**
 * That's maximum 30 seconds of runtime in microseconds
 */
int64_t maxTimeRunning = 30000000;
double_t get_battery_percentage() {
  // When reading the battery voltage, POWER_EN must be turned on
  epd_poweron();
  delay(50);
  Serial.println(epd_ambient_temperature());
  uint16_t v = analogRead(BATT_PIN);
  Serial.print("Battery analogRead value is");
  Serial.println(v);
  double_t battery_voltage =
      ((double_t)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
  // Better formula needed I suppose
  // experimental super simple percent estimate no lookup anything just divide
  // by 100
  double_t percent_experiment = ((battery_voltage - 3.7) / 0.5) * 100;
  // cap out battery at 100%
  // on charging it spikes higher
  if (percent_experiment > 100) {
    percent_experiment = 100;
  }
  String voltage = "Battery Voltage :" + String(battery_voltage) +
                   "V which is around " + String(percent_experiment) + "%";
  Serial.println(voltage);
  epd_poweroff();
  delay(50);
  return percent_experiment;
}
void display_center_message(const char *text) {
  // first set full screen to white
  epd_hl_set_all_white(&hl);
  int cursor_x = EPD_WIDTH / 2;
  int cursor_y = EPD_HEIGHT / 2;
  if (orientation == EPD_ROT_PORTRAIT) {
    // height and width switched here because portrait mode
    cursor_x = EPD_HEIGHT / 2;
    cursor_y = EPD_WIDTH / 2;
  }
  EpdFontProperties font_props = epd_font_properties_default();
  font_props.flags = EPD_DRAW_ALIGN_CENTER;
  epd_write_string(&FiraMono, text, &cursor_x, &cursor_y, fb, &font_props);
  epd_poweron();
  err = epd_hl_update_screen(&hl, MODE_GC16, temperature);
  delay(500);
  epd_poweroff();
  delay(1000);
}
void display_full_screen_left_aligned_text(const char *text) {
  EpdFontProperties font_props = epd_font_properties_default();
  font_props.flags = EPD_DRAW_ALIGN_LEFT;
  // first set full screen to white
  epd_hl_set_all_white(&hl);
  /************* Display the text itself ******************/
  // hardcoded to start at upper left corner
  // with bit of padding
  int cursor_x = 10;
  int cursor_y = 30;
  epd_write_string(&FiraMono, text, &cursor_x, &cursor_y, fb, &font_props);
  /********************************************************/
  /************ Battery percentage display ****************/
  // height and width switched here because portrait mode
  int battery_cursor_x = EPD_WIDTH - 30;
  int battery_cursor_y = EPD_HEIGHT - 10;
  if (orientation == EPD_ROT_PORTRAIT) {
    // switched x and y constants in portrait mode
    battery_cursor_x = EPD_HEIGHT - 10;
    battery_cursor_y = EPD_WIDTH - 30;
  }
  EpdFontProperties battery_font_props = epd_font_properties_default();
  battery_font_props.flags = EPD_DRAW_ALIGN_RIGHT;
  String battery_text = String(get_battery_percentage());
  battery_text.concat("% Battery");
  epd_write_string(&FiraMono, battery_text.c_str(), &battery_cursor_x,
                   &battery_cursor_y, fb, &battery_font_props);
  /********************************************************/
  epd_poweron();
  err = epd_hl_update_screen(&hl, MODE_GC16, temperature);
  delay(500);
  epd_poweroff();
  delay(1000);
}
/**
 * Powers off everything into deepsleep so device and display.
 */
void start_deep_sleep_with_wakeup_sources() {
  epd_poweroff();
  delay(400);
  esp_sleep_enable_ext0_wakeup(FIRST_BTN_PIN, 0);
  Serial.println("Sending device to deepsleep");
  esp_deep_sleep_start();
}
/**
 * Function that prints the reason by which ESP32 has been awaken from sleep
 */
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch (wakeup_reason) {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}
/**
 * Correct the ADC reference voltage. Was in example of lilygo epd47 repository
 * to calc battery percentage
 */
void correct_adc_reference() {
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
      ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
    vref = adc_chars.vref;
  }
}
void setup() {
  Serial.begin(115200);
  correct_adc_reference();
  // First setup epd to use later
  epd_init(EPD_OPTIONS_DEFAULT);
  hl = epd_hl_init(WAVEFORM);
  epd_set_rotation(orientation);
  fb = epd_hl_get_framebuffer(&hl);
  epd_clear();
  print_wakeup_reason();
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Woken up by wakeup source button");
    pressed_wakeup_btn_index++;
    String message = String("Woken up from deepsleep times: ");
    message.concat(String(pressed_wakeup_btn_index));
    display_full_screen_left_aligned_text(message.c_str());
  } else {
    /* Non deepsleep wakeup source button interrupt caused start e.g. reset btn
     */
    Serial.println("Woken up by reset button or power cycle");
    const char *message =
        "Hello! You just turned me on.\nIn 30s I will go to deepsleep";
    display_center_message(message);
  }
}
void loop() {
  /*
   * Shutdown device after 30s always to conserve battery.
   * Basically almost no battery usage then until next wakeup.
   */
  if (esp_timer_get_time() > maxTimeRunning) {
    Serial.println(
        "Max runtime of 30s reached. Forcing deepsleep now to save energy");
    display_center_message("Sleeping now.\nWake me up from deepsleep "
                           "again\nwith the first button on my side");
    delay(1500);
    start_deep_sleep_with_wakeup_sources();
  }
}
//
// Valuable stuff below here, if nothing else the battery monitor.
// Shit, that means it needs wiring directly.. and I've burnt the fucking pads
// off, lol
//
//
// #ifndef BOARD_HAS_PSRAM
// #error "Please enable PSRAM !!!"
// #endif
// #include <Arduino.h>
// // Happily includes from lib/package/src/ if necessary
// #include "FiraMono.h"
// #include "epd_driver.h"
// #define BATT_PIN 36
// int x;
// int y;
// uint8_t *framebuffer;
// void drawAreaFromFramebuffer(Rect_t area = epd_full_screen(),
//                              uint8_t *framebuffer = framebuffer) {
//   epd_draw_image(area, framebuffer, BLACK_ON_WHITE);
// }
// void writeYeTerminal(const GFXfont *font, const char *string, int *cursor_x,
//                      int *cursor_y, uint8_t *framebuffer, double lineHeight =
//                      1, bool vertical = false) {
//   // Nicked from epd_driver.h write_string but using write_mode for white
//   text,
//   // line height, bottom-up layout, etc
//   char *token, *newstring, *tofree;
//   if (string == NULL || newstring == NULL) {
//     return;
//   }
//   tofree = newstring = strdup(string);
//   // taken from the strsep manpage
//   int line_start = *cursor_x;
//   while ((token = strsep(&newstring, "\n")) != NULL) {
//     *cursor_x = line_start;
//     write_mode(font, token, cursor_x, cursor_y, framebuffer, WHITE_ON_WHITE,
//                NULL);
//     *cursor_y += font->advance_y * lineHeight;
//   }
//   free(tofree);
// }
// void setup() {
//   Serial.begin(115200);
//   // epd_set_rotation(2);
//   framebuffer =
//       (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
//   if (!framebuffer) {
//     Serial.println("alloc memory failed !!!");
//     while (1)
//       ;
//   }
//   memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
//   epd_init();
//   epd_poweron();
//   epd_clear();
//   epd_fill_rect(0, 0, EPD_WIDTH, EPD_HEIGHT, 0, framebuffer);
//   epd_draw_rect(600, 450, 120, 60, 0, framebuffer);
//   epd_fill_circle(200, 200, 200, 230, framebuffer);
//   epd_fill_circle(300, 300, 200, 210, framebuffer);
//   epd_fill_circle(400, 400, 200, 180, framebuffer);
//   epd_fill_circle(500, 500, 200, 130, framebuffer);
//   epd_fill_circle(600, 600, 200, 0, framebuffer);
//   //
//   // drawAreaFromFramebuffer();
//   //
//   //
//   // writeYeTerminal into buffer is broken for white text. I'd have to:
//   // - fuck about with draw_char, passing mode and figuring the pixel colours
//   // out,
//   // - might as well fix write_string passing mode there to but doesn't look
//   // like they're accepting PRs
//   // - might also as well add like, a line-height param
//   //
//   // x = 300;
//   // y = 300;
//   // writeYeTerminal((GFXfont *)&FiraMono,
//   // "Lots\nand\nlots\nof\njuicy\nnew\nlines\nbaby!", &x, &y, NULL, .8,
//   true);
//   //
//   // AND there's probably a cleverer way of doing this, like
//   // Only writing the bottom line and copy paste dumping the framebuffer
//   pixels
//   // upward
//   //
//   // !! In fact, don't bother continuing with vertical=true until I've had a
//   // thought about this:: !! Full text buffer:
//   // - Draw individual characters along the bottom line
//   // - When \n detected, dump it in buffer above
//   //
//   // Pixels *******:
//   // - Draw individual characters along the bottom line
//   // - When \n detected, draw entire framebuffer y: -30px onto the screen
//   // - Clear bottom line area
//   //
//   // Draw bottom half to top or something
//   // epd_clear();
//   //
//   epd_fill_rect(0, 0, EPD_WIDTH, EPD_HEIGHT / 2, 0, framebuffer);
//   // epd_draw_image(epd_full_screen(), framebuffer, BLACK_ON_WHITE);
//   // delay(2000);
//   // Single lines
//   x = 0;
//   y = 480;
//   char *string0 =
//   "123456789012345678901234567890123456789012345678901234567890"
//                   "12345678901234567890";
//   write_mode((GFXfont *)&FiraMono, string0, &x, &y, framebuffer,
//   BLACK_ON_WHITE,
//              NULL);
//   x = 0;
//   y = 500;
//   char *string1 =
//   "123456789012345678901234567890123456789012345678901234567890"
//                   "12345678901234567890";
//   write_mode((GFXfont *)&FiraMono, string1, &x, &y, framebuffer,
//   WHITE_ON_WHITE,
//              NULL);
//   x = 0;
//   y = 530;
//   char *string2 =
//   "123456789012345678901234567890123456789012345678901234567890"
//                   "12345678901234567890";
//   write_mode((GFXfont *)&FiraMono, string2, &x, &y, framebuffer,
//   WHITE_ON_BLACK,
//              NULL);
//   //
//   epd_draw_image(epd_full_screen(), framebuffer, WHITE_ON_BLACK);
//   delay(3000);
//   //
//   // Fill top black
//   epd_fill_rect(0, 0, EPD_WIDTH, EPD_HEIGHT / 2, 255, framebuffer);
//   delay(1000);
//   // Redraw top half only
//   drawAreaFromFramebuffer(Rect_t{
//       .x = 0,
//       .y = 0,
//       .width = EPD_WIDTH,
//       .height = EPD_HEIGHT / 2,
//   });
//   delay(3000);
//   // Copy screen offset vertically up
//   epd_draw_image(
//       Rect_t{
//           .x = 0,
//           .y = -EPD_HEIGHT / 2,
//           .width = EPD_WIDTH,
//           .height = EPD_HEIGHT,
//       },
//       framebuffer, BLACK_ON_WHITE);
//   delay(3000);
//   // Redraw everything
//   drawAreaFromFramebuffer();
//   epd_poweroff();
//   delay(5000);
// }
// void batteryMeter() {
//   uint16_t v = analogRead(BATT_PIN);
//   // (From demo code 🤷🏼‍♀️)
//   int vref = 1100;
//   float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
//   String voltage = String(battery_voltage) + "V\nfoo\nbar\nbaz";
//   int x = 0;
//   int y = 20;
//   // int x = EPD_WIDTH - 100;
//   // int y = 0;
//   epd_clear_area(Rect_t{
//       .x = x,
//       .y = y,
//       .width = 100,
//       .height = 50,
//   });
//   write_mode((GFXfont *)&FiraMono, (char *)voltage.c_str(), &x, &y, NULL,
//              WHITE_ON_WHITE, NULL);
// }
// void loop() {
//   // epd_poweron();
//   // batteryMeter();
//   // epd_poweroff_all();
//   // delay(5000);
// }