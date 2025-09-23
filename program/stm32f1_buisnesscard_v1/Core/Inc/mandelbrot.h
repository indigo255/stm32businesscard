#pragma once

#define RES_X             160
#define RES_Y             80

struct camera {
  double min_r, min_i, max_r, max_i;
};

struct window {
  unsigned int x0, y0, w, h;
};

void render_loop();
void init_colorscheme(uint16_t *scheme);
unsigned int mandelbrot_bordertrace(uint16_t *framebuffer, const uint16_t *colorscheme, const struct camera cam, const struct window win);
unsigned int mandelbrot_unoptimized(uint16_t *framebuffer, const uint16_t *colorscheme, const struct camera cam, const struct window win);
