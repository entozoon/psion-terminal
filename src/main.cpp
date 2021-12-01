#include "Arduino.h"
#include "Firasans.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "epd_driver.h"
#include "epd_highlevel.h"
// #include "esp_heap_caps.h"
// #include "esp_log.h"
// #include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// Compile as C
// https://community.platformio.org/t/private-lib-undefined-reference/17475/3
extern "C" {
#include "st.h"
}
// Note USB https://github.com/vroland/epdiy/issues/12#issuecomment-761029851
#define USB_TXD (GPIO_NUM_14)
#define SERIAL_RXD (GPIO_NUM_15)
#define BUF_SIZE (4096)
#define ESC_BUF_SIZE (128 * 4)
#define ESC_ARG_SIZE 16
#define BATT_PIN 36
#define WAVEFORM EPD_BUILTIN_WAVEFORM
EpdiyHighlevelState hl;
uint8_t *fb;
// EpdRotation orientation = EPD_ROT_INVERTED_LANDSCAPE;
EpdRotation orientation = EPD_ROT_LANDSCAPE;
int vref = 1100;
RTC_DATA_ATTR int pressed_wakeup_btn_index;
int64_t maxTimeRunning = 30000000;
void csihandle(void);
void tclearregion(int x1, int y1, int x2, int y2);
int min(int a, int b) { return a < b ? a : b; }
int max(int a, int b) { return a > b ? a : b; }
void read_task(void *argh) {
  while (true) {
    ttyread();
  }
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
  epd_write_string(&FiraSans_12, text, &cursor_x, &cursor_y, fb, &font_props);
  epd_poweron();
  delay(500);
  epd_poweroff();
  delay(1000);
}
void setup() {
  Serial.begin(115200);
  // First setup epd to use later
  epd_init(EPD_OPTIONS_DEFAULT);
  delay(500);
  hl = epd_hl_init(WAVEFORM);
  delay(500);
  epd_set_rotation(orientation);
  fb = epd_hl_get_framebuffer(&hl);
  delay(500);
  epd_clear();
  epd_poweron();
  display_center_message("Hello?");
  epd_poweroff();
  uart_config_t uart_config;
  uart_config.baud_rate = 115200;
  uart_config.data_bits = UART_DATA_8_BITS;
  uart_config.parity = UART_PARITY_DISABLE;
  uart_config.stop_bits = UART_STOP_BITS_1;
  uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  uart_config.use_ref_tick = true;
  uart_param_config(UART_NUM_1, &uart_config);
  // Change here to modify tx/rx pins
  ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, USB_TXD, SERIAL_RXD,
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, BUF_SIZE, 1024, 0, NULL, 0));
  // Still log to the serial output
  // Needed if reusing the USB TX/RX
  // esp_log_set_vprintf(log_to_uart);
  ESP_LOGI("term", "listening\n");
  tnew(cols, rows); // dies here
  selinit();
  RTOS_ERROR_CHECK(
      xTaskCreatePinnedToCore(&read_task, "read", 1 << 12, NULL, 1, NULL, 0));
}
void loop() {}