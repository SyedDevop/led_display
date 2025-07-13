#include <hardware/gpio.h>
#include <pico/assert.h>
#include <pico/time.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "ws2812.pio.h"

#define GRID_WIDTH 8
#define GRID_HEIGHT 8
#define PANEL_COUNT 1
#define NUM_PIXEL ((GRID_WIDTH * GRID_HEIGHT) * PANEL_COUNT)
#define LED_PIN 2

static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
  pio_sm_put_blocking(pio, sm, pixel_grb << 8);
}

static inline uint32_t rgb_u32(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

static inline uint32_t rgba_u32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  // Scale RGB values by alpha (a/255) using integer math
  // Multiply by a and divide by 255 to get scaled value
  uint16_t scaled_r = ((uint16_t)r * a) / 255;
  uint16_t scaled_g = ((uint16_t)g * a) / 255;
  uint16_t scaled_b = ((uint16_t)b * a) / 255;
  return rgb_u32((uint8_t)scaled_r, (uint8_t)scaled_g, (uint8_t)scaled_b);
}

static inline void clear_all(PIO pio, uint sm) {
  for (int i = 0; i < NUM_PIXEL; i++) {
    put_pixel(pio, sm, 0x000000);
  }
}

static uint32_t pixels[NUM_PIXEL] = {0};

static inline void put_pixel_buff(int index, uint32_t color) {
  pixels[index] = color;
}

static inline void clear_buff() {
  memset(&pixels, 0, sizeof(uint32_t) * NUM_PIXEL);
}

static inline void show_pixel(PIO pio, uint sm) {
  for (int i = 0; i < NUM_PIXEL; i++) {
    put_pixel(pio, sm, pixels[i]);
  }
}

int main() {
  stdio_init_all();
  sleep_ms(1000);
  printf("Starting, ws2812 with %d LEDs using pin %d\n", NUM_PIXEL, LED_PIN);

  // // Initialise the Wi-Fi chip
  if (cyw43_arch_init()) {
    printf("Wi-Fi init failed\n");
    return -1;
  }
  // Example to turn on the Pico W LED
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

  PIO pio;
  uint sm;
  uint offset;
  bool success = pio_claim_free_sm_and_add_program_for_gpio_range(
      &ws2812_program, &pio, &sm, &offset, LED_PIN, 1, true);
  hard_assert(success);

  ws2812_program_init(pio, sm, offset, LED_PIN, 800000, false);

  while (true) {
    for (int i = 0; i < NUM_PIXEL; i++) {
      put_pixel_buff(i, i % 2 == 0 ? rgba_u32(255, 0, 0, 1)
                                   : rgba_u32(0, 255, 0, 1));

      show_pixel(pio, sm);
      sleep_ms(100);
    }
    for (int i = NUM_PIXEL; i > 0; i--) {
      put_pixel_buff(i, 0);
      show_pixel(pio, sm);
      sleep_ms(100);
    }
    clear_buff();
    show_pixel(pio, sm);
    sleep_ms(500);
  }

  pio_remove_program_and_unclaim_sm(&ws2812_program, pio, sm, offset);
}
