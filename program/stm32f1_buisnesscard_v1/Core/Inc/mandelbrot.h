#pragma once

extern int state;
enum states {
  MANDELBROT,
  BURNINGSHIP,
  SCREENSAVER,
  HELP,
};

void draw_mandelbrot(int key_pressed);
