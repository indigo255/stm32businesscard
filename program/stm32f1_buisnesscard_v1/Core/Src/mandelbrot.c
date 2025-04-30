#include <stdint.h>
#include "mandelbrot.h"
#include "st7735.h"

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

int state = MANDELBROT; //TODO change to help once developed

struct camera {
  double min_r, min_i, max_r, max_i;
};

#undef SHIP

//TODO optimize the FUCK outta this, it's ass
#ifdef SHIP
//BGR coloring, b:5 x g:6 x r:5
inline uint16_t get_color(int i) {
  if(i == 0) || (i == ITERS) return 0;
  //return (2*(i - (128)+0x1f) << (5+6); FIXME
    
}
#else
uint16_t get_color(int i);
inline uint16_t get_color(int i) {
  if((i == 0) || (i == ITERS)) return 0;
  //return(ST7735_BLUE);
  //TODO convert multiplication to shifts
  if(i < 128) return (((4*(i - 64)+0x1f) & 0x1f) | (((2*(i - 128)+0x1f) & 0x1f) << (5+6)));
  return (-2*(i - 128)+0x1f) & 0xff;
}
#endif

void shift_cam(struct camera *cam, double step_r, double step_i) {
  double i_offset = (cam->max_i - cam->min_i) * step_i;
  double r_offset = (cam->max_r - cam->min_r) * step_r;
  cam->min_i += i_offset;
  cam->max_i += i_offset;
  cam->min_r += r_offset;
  cam->max_r += r_offset;
}

void zoom_cam(struct camera *cam, double zoom) {
  double i_scale = (cam->max_i - cam->min_i) * zoom;
  double r_scale = (cam->max_r - cam->min_r) * zoom;
  cam->min_i += i_scale;
  cam->max_i -= i_scale;
  cam->min_r += r_scale;
  cam->max_r -= r_scale;
}

void draw_mandelbrot(int key_pressed) {
  static struct camera cam = {
    .min_r = CAM_DEF_MIN_R,
    .max_r = CAM_DEF_MAX_R,
    .min_i = ((double)RES_Y / RES_X) * CAM_DEF_MIN_R,
    .max_i = ((double)RES_Y / RES_X) * CAM_DEF_MAX_R,

  };
  switch(key_pressed) {
    case KEYCODE_UP:
      shift_cam(&cam, 0, STEP_SIZE);
      break;
    case KEYCODE_DOWN:
      shift_cam(&cam, 0, -STEP_SIZE);
      break;
    case KEYCODE_RIGHT:
      shift_cam(&cam, -STEP_SIZE, 0);
      break;
    case KEYCODE_LEFT:
      shift_cam(&cam, STEP_SIZE, 0);
      break;
    case KEYCODE_A:
      zoom_cam(&cam, ZOOM_SIZE);
      break;
    case KEYCODE_B:
      zoom_cam(&cam, -ZOOM_SIZE);
      break;
    default:
  }
  {
      int32_t scale_i = DOUBLE_TO_FIXED((cam.max_i - cam.min_i) / (double)RES_Y);
      int32_t scale_r = DOUBLE_TO_FIXED((cam.max_r - cam.min_r) / (double)RES_X);
      int32_t c_i = DOUBLE_TO_FIXED(cam.max_i);
      int32_t c_r, z_i, z_r, zn_i, zn_r;
      int32_t z_r_2, z_i_2;

      int i;
      for(int y = 0; y < RES_Y; y++) {
        c_r = DOUBLE_TO_FIXED(cam.min_r);
        for(int x = 0; x < RES_X; x++) {
          z_i = 0;
          z_r = 0;
          //TODO optimize
          for(i = 0; i < ITERS; i++) {
            z_r_2 = FIXED_MULTIPLY(z_r, z_r);
            z_i_2 = FIXED_MULTIPLY(z_i, z_i);

            zn_r = z_r_2 - z_i_2 + c_r;

#ifdef SHIP
            zn_i = abs(FIXED_MULTIPLY((DOUBLE_TO_FIXED(2)), (FIXED_MULTIPLY(z_r, z_i)))) + c_i;
#else
            zn_i = (FIXED_MULTIPLY((DOUBLE_TO_FIXED(2)), (FIXED_MULTIPLY(z_r, z_i)))) + c_i;
#endif

            z_i = zn_i;
            z_r = zn_r;

            if(z_i_2 + z_r_2 > INFTY_SQR_FIXED) break;
          }
            ST7735_DrawPixel(x, y, get_color(i));
          c_r += scale_r;
        }
        c_i -= scale_i;
      }
    }
}
