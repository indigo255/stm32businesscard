#pragma once

extern int state;
enum states {
  MANDELBROT,
  BURNINGSHIP,
  SCREENSAVER,
  HELP,
};

void draw_mandelbrot(int key_pressed);
/** the interrupt handler should read the pin pressed, then translate it into one of these. 
 * Ensures that if we have to change pins, adjustments are easy.
 */
enum buttons {
  KEYCODE_UP,
  KEYCODE_RIGHT,
  KEYCODE_DOWN,
  KEYCODE_LEFT,
  KEYCODE_A,
  KEYCODE_B,
  //we need to call the function once during boot
  KEYCODE_NONE = 0xff 
};
