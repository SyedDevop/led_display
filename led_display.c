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
#define num_to_char(num) (num + 48)

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
#define DIGIT_FONT_WIDTH 4
#define DIGIT_FONT_HEIGHT 6

#define LETTER_FONT_WIDTH 5
#define LETTER_FONT_HEIGHT 5

typedef bool DIGIT_FONT[DIGIT_FONT_WIDTH * DIGIT_FONT_HEIGHT];
typedef bool LETTER_FONT[LETTER_FONT_WIDTH * LETTER_FONT_HEIGHT];

static const DIGIT_FONT DIGIT_FONTS[10] = {
    // 0
    {0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0},
    // 1
    {0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1},
    // 2
    {0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1},
    // 3
    {1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0},
    // 4
    {0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0},
    // 5
    {1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0},
    // 6
    {0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0},
    // 7
    {1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0},
    // 8
    {0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0},
    // 9
    {0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0},
};

static const LETTER_FONT LETTER_FONTS[26] = {
    // a
    {0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1},
    // b
    {1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0},
    // c
    {0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0},
    // d
    {1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0},
    // e
    {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1},
    // f
    {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0},
    // g
    {0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0},
    // h
    {1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1},
    // i
    {1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1},
    // j
    {0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0},
    // k
    {1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0},
    // l
    {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1},
    // m
    {1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1},
    // n
    {1, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1},
    // o
    {0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0},
    // p
    {1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0},
    // q
    {0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1},
    // r
    {1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1},
    // s
    {0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0},
    // t
    {1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0},
    // u
    {1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0},
    // v
    {1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0},
    // w
    {1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1},
    // x
    {1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1},
    // y
    {1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0},
    // z
    {1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1},
};

enum Rotation { Ninety, None };

static inline void put_digit_in_pixel_buff(const DIGIT_FONT font, int base_x,
                                           int base_y, uint32_t color,
                                           enum Rotation rotate_led) {
  int pixel_index, x, y = 0;
  // Row == y and col == x
  //       v -> y
  for (int row = 0; row < DIGIT_FONT_HEIGHT; ++row) {
    //       v -> x
    for (int col = 0; col < DIGIT_FONT_WIDTH; ++col) {
      int font_index = row * DIGIT_FONT_WIDTH + col;
      if (!font[font_index])
        continue;
      switch (rotate_led) {
      case Ninety:
        x = DIGIT_FONT_WIDTH - 1 - col;
        y = DIGIT_FONT_HEIGHT - 1 - row;
        pixel_index = (base_y + y) * GRID_WIDTH + (base_x + x);
        break;
      case None:
        pixel_index = (base_y + row) * GRID_WIDTH + (base_x + col);
        break;
      }
      if (pixel_index >= 0 && pixel_index < NUM_PIXEL) {
        pixels[pixel_index] = color;
      }
    }
  }
}

static inline void put_letter_in_pixel_buff(const LETTER_FONT font, int base_x,
                                            int base_y, uint32_t color,
                                            enum Rotation rotate_led) {
  for (int row = 0; row < LETTER_FONT_HEIGHT; ++row) {
    for (int col = 0; col < LETTER_FONT_WIDTH; ++col) {
      int font_index = row * LETTER_FONT_WIDTH + col;
      if (!font[font_index])
        continue;
      int pixel_index, x, y = 0;
      switch (rotate_led) {
      case Ninety:
        x = DIGIT_FONT_WIDTH - 1 - col;
        y = DIGIT_FONT_HEIGHT - 1 - row;
        pixel_index = (base_y + y) * GRID_WIDTH + (base_x + x + 1);
        break;
      case None:
        pixel_index = (base_y + row) * GRID_WIDTH + (base_x + col);
        break;
      }
      if (pixel_index >= 0 && pixel_index < NUM_PIXEL) {
        pixels[pixel_index] = color;
      }
    }
  }
}

static inline void put_char_in_pixel_buff(char c, int base_x, int base_y,
                                          uint32_t color) {
  if (c >= '0' && c <= '9') {
    put_digit_in_pixel_buff(DIGIT_FONTS[c - '0'], base_x, base_y, color, None);
  } else if (c >= 'A' && c <= 'Z') {
    put_letter_in_pixel_buff(LETTER_FONTS[c - 'A'], base_x, base_y, color,
                             Ninety);
  } else if (c >= 'a' && c <= 'z') {
    put_letter_in_pixel_buff(LETTER_FONTS[c - 'a'], base_x, base_y, color,
                             Ninety);
  }
}

int main() {
  stdio_init_all();
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
    for (int i = 60; i > 0; i--) {

      put_char_in_pixel_buff(num_to_char(i / 10), 0, 0,
                             rgba_u32(50, 100, 150, 10));
      put_char_in_pixel_buff(num_to_char(i % 10), 4, 1,
                             rgba_u32(50, 100, 150, 10));

      show_pixel(pio, sm);
      sleep_ms(100);
      clear_buff();
    }

    for (int i = 0; i < 26; i++) {
      put_char_in_pixel_buff(i + 'a', 0, 0, rgba_u32(50, 100, 150, 10));
      show_pixel(pio, sm);
      sleep_ms(1000);
      clear_buff();
    }

    // put_letter_in_pixel_buff(LETTER_FONTS['l' - 97], 0, 0,
    //                          rgba_u32(50, 100, 150, 10));
    // show_pixel(pio, sm);
    // sleep_ms(500);
    // clear_buff();
  }

  pio_remove_program_and_unclaim_sm(&ws2812_program, pio, sm, offset);
}
