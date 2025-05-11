#include <stdint.h>
#include "mandelbrot.h"
#include "st7735.h"
#include "benchmark.h"

#define RES_X             160
#define RES_Y             80
#define DEFAULT_CENTER_X  0
#define DEFAULT_CENTER_Y  0
#define STEP_SIZE         .1
#define ZOOM_SIZE         .1

#define DECIMAL_LOC           28 //TODO see if you can optimize without DSP module
#define DOUBLE_SCALER         (1 << DECIMAL_LOC)
#define DOUBLE_TO_FIXED(val)  (int32_t)((val) * DOUBLE_SCALER)
#define FIXED_MULTIPLY(x,y)   ((((uint64_t)(x))*(y)) >> DECIMAL_LOC)
#define FIXED_TO_DOUBLE(val)  ((val) / (double)DOUBLE_SCALER)

#define INFTY     2
#define INFTY_SQR INFTY * INFTY
#define ITERS     255
#define INFTY_SQR_FIXED DOUBLE_TO_FIXED(INFTY_SQR)

//TODO move to some hardware.h or somethin
//channel order: B, G, R
#define R_BITS  5
#define G_BITS  6
#define B_BITS  5

//imaginary axis set automatically
#define CAM_DEF_MIN_R -1
#define CAM_DEF_MAX_R  1

struct camera {
  double min_r, min_i, max_r, max_i;
};

enum VIEW_MODES { VIEW_UNINIT, VIEW_MANDREL, VIEW_SHIP };

void init_colorscheme_mandrel(uint16_t *scheme) {
  uint16_t *tc = scheme;
  for(unsigned int i = 0; i < ITERS; i++) {
    if((i == 0) || (i == ITERS)) *tc = 0;
    else if(i < 128) *tc = (((((i - 64) << 2)+0x1f) & 0x1f) | (((((i - 128) << 1)+0x1f) & 0x1f) << (5+6)));
    else *tc = (-2*(i - 128)+0x1f) & 0xff;
    *tc = (*tc << 8) | (*tc >> 8); //convert to little endian
    tc++;
  }
}

void init_colorscheme_ship(uint16_t *scheme) {
  uint16_t *tc = scheme;
  for(unsigned int i = 0; i < ITERS; i++) {
    if((i == 0) || (i == ITERS)) *tc = 0;
    else *tc = (((i - (128)) << 1)+0x1f) << (5+6);
    tc++;
  }
}

void cam_shift(struct camera *cam, double step_r, double step_i) {
  double i_offset = (cam->max_i - cam->min_i) * step_i;
  double r_offset = (cam->max_r - cam->min_r) * step_r;
  cam->min_i += i_offset;
  cam->max_i += i_offset;
  cam->min_r += r_offset;
  cam->max_r += r_offset;
}

void cam_zoom(struct camera *cam, double zoom) {
  double i_scale = (cam->max_i - cam->min_i) * zoom;
  double r_scale = (cam->max_r - cam->min_r) * zoom;
  cam->min_i += i_scale;
  cam->max_i -= i_scale;
  cam->min_r += r_scale;
  cam->max_r -= r_scale;
}

//TODO look into border tracing; this is too slow
void render_mandelbrot(uint16_t *framebuffer, uint16_t *colorscheme, struct camera cam, bool half) {
  int32_t scale_i = DOUBLE_TO_FIXED((cam.max_i - cam.min_i) / (double)RES_Y);
  int32_t scale_r = DOUBLE_TO_FIXED((cam.max_r - cam.min_r) / (double)RES_X);
  int32_t c_i = DOUBLE_TO_FIXED(half ? ((cam.max_i - cam.min_i) / 2) + cam.min_i : (cam.max_i));
  int32_t c_r, z_i, z_r, zn_r;
  int32_t z_r_2, z_i_2;
  size_t fb_index = 0;
  int i; 
  for(int y = 0; y < (RES_Y/2); y++) {
    c_r = DOUBLE_TO_FIXED(cam.min_r);
    for(int x = 0; x < RES_X; x++) {
      z_i = 0;
      z_r = 0;
      for(i = 0; i < ITERS; i++) {
        z_r_2 = FIXED_MULTIPLY(z_r, z_r);
        z_i_2 = FIXED_MULTIPLY(z_i, z_i);

        zn_r = z_r_2 - z_i_2 + c_r;

        //z_i = abs(FIXED_MULTIPLY((DOUBLE_TO_FIXED(2)), (FIXED_MULTIPLY(z_r, z_i)))) + c_i;
        z_i = (FIXED_MULTIPLY(z_r, z_i) << 1)  + c_i;

        z_r = zn_r;

        if(z_i_2 + z_r_2 > INFTY_SQR_FIXED) break;
      }
      framebuffer[fb_index++] = colorscheme[i];
//      ST7735_DrawPixel(x, y, colorscheme[i]);
      c_r += scale_r;
    }
    c_i -= scale_i;
  }
}
  


//TODO rename
void draw_mandelbrot(int key_pressed) {
  uint16_t framebuffer[(RES_X * RES_Y) / 2];
  //program flow is awful atm becuase I was planning something different; will be improved soon.
  static struct camera cam = {
    .min_r = CAM_DEF_MIN_R,
    .max_r = CAM_DEF_MAX_R,
    .min_i = ((double)RES_Y / RES_X) * CAM_DEF_MIN_R,
    .max_i = ((double)RES_Y / RES_X) * CAM_DEF_MAX_R,

  };
  static int view_mode = VIEW_UNINIT;
  static uint16_t colorscheme[ITERS];

  if(view_mode == VIEW_UNINIT) {
    view_mode = VIEW_MANDREL;
    init_colorscheme_mandrel(colorscheme);
  }

  switch(key_pressed) {
    case KEYCODE_UP:
      cam_shift(&cam, 0, STEP_SIZE);
      break;
    case KEYCODE_DOWN:
      cam_shift(&cam, 0, -STEP_SIZE);
      break;
    case KEYCODE_RIGHT:
      cam_shift(&cam, -STEP_SIZE, 0);
      break;
    case KEYCODE_LEFT:
      cam_shift(&cam, STEP_SIZE, 0);
      break;
    case KEYCODE_A:
      cam_zoom(&cam, ZOOM_SIZE);
      break;
    case KEYCODE_B:
      cam_zoom(&cam, -ZOOM_SIZE);
      break;
    default:
  }
  render_mandelbrot(framebuffer, colorscheme, cam, false);
  ST7735_DrawImage(0, 0, ST7735_WIDTH, (ST7735_HEIGHT / 2), framebuffer);
  render_mandelbrot(framebuffer, colorscheme, cam, true);
  ST7735_DrawImage(0, (ST7735_HEIGHT / 2), ST7735_WIDTH, (ST7735_HEIGHT / 2), framebuffer);
//  ST7735_DrawImage(0, 0, ST7735_WIDTH - 1, ST7735_HEIGHT - 1, (uint16_t *)0x80000000);
}
