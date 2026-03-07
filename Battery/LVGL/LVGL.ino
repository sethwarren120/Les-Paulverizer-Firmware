/**
 ******************************************************************************
 * @file     LVGL.ino
 * @author   Rongmin Wu
 * @version  V1.0
 * @date     2024-10-30
 * @brief    LVGL display and touch experiment + Battery % overlay (GPIO1 ADC)
 ******************************************************************************
 */

#include <stdio.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"

#include "lvgl.h"
// #include "demos/lv_demos.h"   // not needed for the battery-only UI
#include "esp_lcd_sh8601.h"
#include "touch_bsp.h"

// --- ADC (ESP-IDF oneshot + optional calibration) ---
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static const char *TAG = "example";
static SemaphoreHandle_t lvgl_mux = NULL;

#define LCD_HOST    SPI2_HOST
#define TOUCH_HOST  I2C_NUM_0

#define LCD_BIT_PER_PIXEL       (16)

#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_LCD_CS            (GPIO_NUM_6)
#define EXAMPLE_PIN_NUM_LCD_PCLK          (GPIO_NUM_47)
#define EXAMPLE_PIN_NUM_LCD_DATA0         (GPIO_NUM_18)
#define EXAMPLE_PIN_NUM_LCD_DATA1         (GPIO_NUM_7)
#define EXAMPLE_PIN_NUM_LCD_DATA2         (GPIO_NUM_48)
#define EXAMPLE_PIN_NUM_LCD_DATA3         (GPIO_NUM_5)
#define EXAMPLE_PIN_NUM_LCD_RST           (GPIO_NUM_17)
#define EXAMPLE_PIN_NUM_BK_LIGHT          (-1)

// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES              536
#define EXAMPLE_LCD_V_RES              240

#define EXAMPLE_USE_TOUCH              0 //Without tp ---- Touch off

#define EXAMPLE_LVGL_BUF_HEIGHT        (EXAMPLE_LCD_V_RES/4)
#define EXAMPLE_LVGL_TICK_PERIOD_MS    2
#define EXAMPLE_LVGL_TASK_MAX_DELAY_MS 500
#define EXAMPLE_LVGL_TASK_MIN_DELAY_MS 1
#define EXAMPLE_LVGL_TASK_STACK_SIZE   (4 * 1024)
#define EXAMPLE_LVGL_TASK_PRIORITY     2

// -------------------- Battery config --------------------
// Board schematic: BAT_ADC is GPIO1, divider is 100k/100k => VBAT ≈ 2 * Vadc.
static const gpio_num_t PIN_BAT_ADC = GPIO_NUM_1;
static const adc_unit_t BAT_ADC_UNIT = ADC_UNIT_1;
static const adc_channel_t BAT_ADC_CH = ADC_CHANNEL_0; // ADC1_CH0 -> GPIO1 on ESP32-S3

static const float VBAT_DIVIDER = 2.0f;   // 100k/100k
static const float VBAT_EMPTY   = 3.20f;  // UI-grade
static const float VBAT_FULL    = 4.20f;

// ADC handles
static adc_oneshot_unit_handle_t adc1_handle = NULL;
static adc_cali_handle_t cali_handle = NULL;
static bool cali_ok = false;

// LVGL battery widgets
static lv_obj_t *label_pct = NULL;
static lv_obj_t *label_v   = NULL;

// -------------------- Panel init cmds --------------------
static const sh8601_lcd_init_cmd_t lcd_init_cmds[] = {
  {0x11, (uint8_t []){0x00}, 0, 120},
  {0x36, (uint8_t []){0xF0}, 1, 0},
  {0x3A, (uint8_t []){0x55}, 1, 0},  //16bits-RGB565
  {0x2A, (uint8_t []){0x00,0x00,0x02,0x17}, 4, 0},
  {0x2B, (uint8_t []){0x00,0x00,0x00,0xEF}, 4, 0},
  {0x51, (uint8_t []){0x00}, 1, 10},
  {0x29, (uint8_t []){0x00}, 0, 10},
  {0x51, (uint8_t []){0xFF}, 1, 0},
};

static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                            esp_lcd_panel_io_event_data_t *edata,
                                            void *user_ctx)
{
  lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
  lv_disp_flush_ready(disp_driver);
  return false;
}

static void example_lvgl_flush_cb(lv_disp_drv_t *drv,
                                  const lv_area_t *area,
                                  lv_color_t *color_map)
{
  esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
  const int offsetx1 = area->x1;
  const int offsetx2 = area->x2;
  const int offsety1 = area->y1;
  const int offsety2 = area->y2;

  esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

static void example_lvgl_update_cb(lv_disp_drv_t *drv)
{
  esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;

  switch (drv->rotated)
  {
    case LV_DISP_ROT_NONE:
      esp_lcd_panel_swap_xy(panel_handle, false);
      esp_lcd_panel_mirror(panel_handle, true, false);
      break;
    case LV_DISP_ROT_90:
      esp_lcd_panel_swap_xy(panel_handle, true);
      esp_lcd_panel_mirror(panel_handle, true, true);
      break;
    case LV_DISP_ROT_180:
      esp_lcd_panel_swap_xy(panel_handle, false);
      esp_lcd_panel_mirror(panel_handle, false, true);
      break;
    case LV_DISP_ROT_270:
      esp_lcd_panel_swap_xy(panel_handle, true);
      esp_lcd_panel_mirror(panel_handle, false, false);
      break;
  }
}

void example_lvgl_rounder_cb(struct _lv_disp_drv_t *disp_drv, lv_area_t *area)
{
  uint16_t x1 = area->x1;
  uint16_t x2 = area->x2;
  uint16_t y1 = area->y1;
  uint16_t y2 = area->y2;

  area->x1 = (x1 >> 1) << 1;
  area->y1 = (y1 >> 1) << 1;
  area->x2 = ((x2 >> 1) << 1) + 1;
  area->y2 = ((y2 >> 1) << 1) + 1;
}

#if EXAMPLE_USE_TOUCH
static void example_lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
  uint16_t tp_x,tp_y;
  uint8_t win;
  win = getTouch(&tp_x,&tp_y);
  if (win)
  {
    data->point.x = tp_x;
    data->point.y = tp_y;
    data->state = LV_INDEV_STATE_PRESSED;
  }
  else
  {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}
#endif

static void example_increase_lvgl_tick(void *arg)
{
  lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

static bool example_lvgl_lock(int timeout_ms)
{
  assert(lvgl_mux && "bsp_display_start must be called first");
  const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
  return xSemaphoreTake(lvgl_mux, timeout_ticks) == pdTRUE;
}

static void example_lvgl_unlock(void)
{
  assert(lvgl_mux && "bsp_display_start must be called first");
  xSemaphoreGive(lvgl_mux);
}

static void example_lvgl_port_task(void *arg)
{
  ESP_LOGI(TAG, "Starting LVGL task");
  uint32_t task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
  while (1)
  {
    if (example_lvgl_lock(-1)) {
      task_delay_ms = lv_timer_handler();
      example_lvgl_unlock();
    }
    if (task_delay_ms > EXAMPLE_LVGL_TASK_MAX_DELAY_MS) task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
    else if (task_delay_ms < EXAMPLE_LVGL_TASK_MIN_DELAY_MS) task_delay_ms = EXAMPLE_LVGL_TASK_MIN_DELAY_MS;

    vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
  }
}

// -------------------- Battery functions --------------------
static void battery_adc_init()
{
  // Create ADC oneshot unit
  adc_oneshot_unit_init_cfg_t init_config1 = {
    .unit_id = BAT_ADC_UNIT,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

  // Configure channel
  adc_oneshot_chan_cfg_t config = {
    .atten = ADC_ATTEN_DB_12,
    .bitwidth = ADC_BITWIDTH_12,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, BAT_ADC_CH, &config));

  // Try calibration (curve fitting). If not supported by your core build, we fall back.
  adc_cali_curve_fitting_config_t cali_config = {
    .unit_id = BAT_ADC_UNIT,
    .chan = BAT_ADC_CH,
    .atten = ADC_ATTEN_DB_12,
    .bitwidth = ADC_BITWIDTH_12,
  };

  if (adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle) == ESP_OK) {
    cali_ok = true;
    ESP_LOGI(TAG, "ADC calibration enabled");
  } else {
    cali_ok = false;
    ESP_LOGW(TAG, "ADC calibration not available; using raw->volts approx");
  }
}

static float battery_read_vbat(int *raw_out)
{
  int raw = 0;
  esp_err_t err = adc_oneshot_read(adc1_handle, BAT_ADC_CH, &raw);
  if (err != ESP_OK) {
    if (raw_out) *raw_out = 0;
    return 0.0f;
  }
  if (raw_out) *raw_out = raw;

  float v_adc = 0.0f;

  if (cali_ok) {
    int mv = 0;
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw, &mv));
    v_adc = (float)mv / 1000.0f;
  } else {
    // Fallback conversion (approx): assumes 3.3V full-scale at 12-bit
    v_adc = ((float)raw * 3.3f) / 4096.0f;
  }

  return v_adc * VBAT_DIVIDER;
}

static int battery_volts_to_percent(float vbat)
{
  if (vbat <= VBAT_EMPTY) return 0;
  if (vbat >= VBAT_FULL)  return 100;

  float p = (vbat - VBAT_EMPTY) / (VBAT_FULL - VBAT_EMPTY);
  int pct = (int)lroundf(p * 100.0f);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

static void battery_ui_create()
{
  // simple screen
  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

  label_pct = lv_label_create(scr);
  lv_obj_set_style_text_color(label_pct, lv_color_white(), 0);
  lv_obj_set_style_text_font(label_pct, &lv_font_montserrat_48, 0);
  lv_label_set_text(label_pct, "Battery --%");
  lv_obj_align(label_pct, LV_ALIGN_CENTER, 0, -30);

  label_v = lv_label_create(scr);
  lv_obj_set_style_text_color(label_v, lv_color_white(), 0);
  lv_obj_set_style_text_font(label_v, &lv_font_montserrat_24, 0);
  lv_label_set_text(label_v, "VBAT --.-- V");
  lv_obj_align(label_v, LV_ALIGN_CENTER, 0, 35);
}

static void battery_ui_update()
{
  if (!label_pct || !label_v) return;

  int raw = 0;
  float vbat = battery_read_vbat(&raw);
  int pct = battery_volts_to_percent(vbat);

  char a[32], b[32];
  snprintf(a, sizeof(a), "Battery %d%%", pct);
  snprintf(b, sizeof(b), "VBAT %.2f V", vbat);

  lv_label_set_text(label_pct, a);
  lv_label_set_text(label_v, b);

  // Optional: log to serial
  ESP_LOGI(TAG, "ADC raw=%d, VBAT=%.3fV, %d%%", raw, vbat, pct);
}

// LVGL-safe update task (runs alongside your lv_timer_handler task)
static void battery_task(void *arg)
{
  (void)arg;
  while (1) {
    if (example_lvgl_lock(-1)) {
      battery_ui_update();
      example_lvgl_unlock();
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup()
{
  static lv_disp_draw_buf_t disp_buf;
  static lv_disp_drv_t disp_drv;

#if EXAMPLE_PIN_NUM_BK_LIGHT >= 0
  ESP_LOGI(TAG, "Turn off LCD backlight");
  gpio_config_t bk_gpio_config = {
    .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT,
    .mode = GPIO_MODE_OUTPUT,
  };
  ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
#endif

  ESP_LOGI(TAG, "Initialize SPI bus");
  const spi_bus_config_t buscfg = SH8601_PANEL_BUS_QSPI_CONFIG(
    EXAMPLE_PIN_NUM_LCD_PCLK,
    EXAMPLE_PIN_NUM_LCD_DATA0,
    EXAMPLE_PIN_NUM_LCD_DATA1,
    EXAMPLE_PIN_NUM_LCD_DATA2,
    EXAMPLE_PIN_NUM_LCD_DATA3,
    EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * LCD_BIT_PER_PIXEL / 8
  );
  ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

  ESP_LOGI(TAG, "Install panel IO");
  esp_lcd_panel_io_handle_t io_handle = NULL;
  const esp_lcd_panel_io_spi_config_t io_config = SH8601_PANEL_IO_QSPI_CONFIG(
    EXAMPLE_PIN_NUM_LCD_CS,
    example_notify_lvgl_flush_ready,
    &disp_drv
  );

  sh8601_vendor_config_t vendor_config = {
    .init_cmds = lcd_init_cmds,
    .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
    .flags = { .use_qspi_interface = 1 },
  };

  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

  esp_lcd_panel_handle_t panel_handle = NULL;
  const esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
    .bits_per_pixel = LCD_BIT_PER_PIXEL,
    .vendor_config = &vendor_config,
  };

  ESP_LOGI(TAG, "Install SH8601 panel driver");
  ESP_ERROR_CHECK(esp_lcd_new_panel_sh8601(io_handle, &panel_config, &panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

#if EXAMPLE_USE_TOUCH
  Touch_Init();
#endif

  ESP_LOGI(TAG, "Initialize LVGL library");
  lv_init();

  lv_color_t *buf1 = (lv_color_t*)heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_DMA);
  assert(buf1);
  lv_color_t *buf2 = (lv_color_t*)heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_DMA);
  assert(buf2);

  lv_disp_draw_buf_init(&disp_buf, buf1, buf2, EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT);

  ESP_LOGI(TAG, "Register display driver to LVGL");
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = EXAMPLE_LCD_H_RES;
  disp_drv.ver_res = EXAMPLE_LCD_V_RES;
  disp_drv.flush_cb = example_lvgl_flush_cb;
  disp_drv.rounder_cb = example_lvgl_rounder_cb;
  disp_drv.drv_update_cb = example_lvgl_update_cb;
  disp_drv.draw_buf = &disp_buf;
  disp_drv.user_data = panel_handle;
  lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

  ESP_LOGI(TAG, "Install LVGL tick timer");
  const esp_timer_create_args_t lvgl_tick_timer_args = {
    .callback = &example_increase_lvgl_tick,
    .name = "lvgl_tick"
  };
  esp_timer_handle_t lvgl_tick_timer = NULL;
  ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

#if EXAMPLE_USE_TOUCH
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.disp = disp;
  indev_drv.read_cb = example_lvgl_touch_cb;
  lv_indev_drv_register(&indev_drv);
#endif

  lvgl_mux = xSemaphoreCreateMutex();
  assert(lvgl_mux);

  // Start LVGL handler task
  xTaskCreate(example_lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, NULL);

  // ---- Battery init + UI ----
  battery_adc_init();

  if (example_lvgl_lock(-1))
  {
    battery_ui_create();   // replace the demo with battery screen
    example_lvgl_unlock();
  }

  // Battery update task
  xTaskCreate(battery_task, "BAT", 3072, NULL, 2, NULL);
}

void loop()
{
  // not used (tasks do all the work)
}