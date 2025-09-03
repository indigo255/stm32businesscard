#include <stdint.h>
#include <string.h>
#include "mandelbrot.h"
#include "st7735.h"
#include "main.h"
#include "gpio.h"
#include "idle.h"

#define RES_X             160
#define RES_Y             80
#define DEFAULT_CENTER_X  0
#define DEFAULT_CENTER_Y  0
#define STEP_SIZE         .1
#define ZOOM_SIZE         .1

#define DECIMAL_LOC           28
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

#define G_MASK  0xe007

//imaginary axis set automatically
#define CAM_DEF_MIN_R -1
#define CAM_DEF_MAX_R  1

//set controls 
#define CAM_MOVE_UP     BUTTON_UP     
#define CAM_MOVE_RIGHT  BUTTON_RIGHT
#define CAM_MOVE_DOWN   BUTTON_DOWN   
#define CAM_MOVE_LEFT   BUTTON_LEFT   
#define CAM_ZOOM_IN     BUTTON_A      
#define CAM_ZOOM_OUT    BUTTON_B      


#define BACKSTACK_SIZE      32
#define GCHAN_UNRENDERED    0 //don't change; green channel zero'd on cam move
#define GCHAN_BLOCKED       (1 << 2) //interior element or visiteed
#define GCHAN_INTERNAL      (1 << 1) //part of set, 0x20
#define GCHAN_EXTERNAL      (1 << 0) //not part of set, 0x10

enum DIRECTIONS {
  N, NE, E, SE, S, SW, W, NW
};

typedef struct {
  int32_t r; int32_t i;
} FixedCord;

struct camera {
  double min_r, min_i, max_r, max_i;
};

struct window {
  unsigned int x0, y0, w, h;
};

//C does remainder, not modulo.
//TODO optimize for mod 8. Benchmark
inline int mod(int n, int d) {
  int r = n % d;
  return (r < 0) ? r + d : r;
}
int mod(int n, int d);


inline FixedCord get_neighbor_coord(FixedCord from_coord, int direction, FixedCord step) {
  if((direction == NW) || (direction < E)) from_coord.i += step.i;  //up
  if((direction > N) && (direction < S)) from_coord.r += step.r;    //right
  if((direction > E) && (direction < W)) from_coord.i -= step.i;    //down
  if(direction > S) from_coord.r -= step.r;                         //left
  return from_coord;
}
FixedCord get_neighbor_coord(FixedCord from_coord, int direction, FixedCord step);


size_t get_neighbor_index(size_t from_pixel, int direction, struct window win) {
  //TODO gross since window is no longer constant
  int neighbor_index_accl[8] = 
    {-win.w, -win.w + 1, 1, win.w + 1, win.w, win.w - 1, -1, -win.w - 1};
  from_pixel += neighbor_index_accl[direction];
  return from_pixel;
}

void detect_borders(bool borders[8], size_t i, struct window win) {
  //if this is too slow, it's easy to do it without the modulos.
  int index_mod = i % win.w;
  bzero(borders, sizeof(*borders) * 8);
  if((i + win.w) >= (win.w * win.h)) {
    for(int nei_dir = SE; nei_dir <= SW; nei_dir++)
      borders[nei_dir] = true;
  }
  else if(((int)(i - win.w)) < 0) {
    borders[NE] = true;
    borders[N] = true;
    borders[NW] = true;
  }
  if(index_mod == 0) {
    for(int nei_dir = SW; nei_dir <= NW; nei_dir++)
      borders[nei_dir] = true;
  }
  else if(index_mod == (win.w - 1)) {
    for(int nei_dir = NE; nei_dir <= SE; nei_dir++)
      borders[nei_dir] = true;
  }
}

enum VIEW_MODES { VIEW_UNINIT, VIEW_MANDREL, VIEW_SHIP };

void init_colorscheme(uint16_t *scheme) {
  uint16_t *tc = scheme;
  for(unsigned int i = 0; i <= ITERS; i++) {
    if(i < 128) *tc = (((((i - 64) << 2)+0x1f) & 0x1f) | (((((i - 128) << 1)+0x1f) & 0x1f) << (5+6)));
    else *tc = (-2*(i - 128)+0x1f) & 0xff;
    *tc = (*tc << 8) | (*tc >> 8); //convert to little endian
    *tc &= ~0b111;                                 
    tc++;
  }
  scheme[0] = 0;
  scheme[ITERS] = 0;
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

inline int  __attribute__((always_inline)) iterate(FixedCord c) {
  int32_t z_i = 0;
  int32_t z_r = 0;
  int32_t z_r_2, z_i_2, zn_r, zn_i;
  for(int it = 0; it < ITERS; it++) {
    z_r_2 = FIXED_MULTIPLY(z_r, z_r);
    z_i_2 = FIXED_MULTIPLY(z_i, z_i);

    zn_r = z_r_2 - z_i_2 + c.r;

    //zn_i = abs(FIXED_MULTIPLY((DOUBLE_TO_FIXED(2)), (FIXED_MULTIPLY(z_r, z_i)))) + c.i;
    zn_i = (FIXED_MULTIPLY((DOUBLE_TO_FIXED(2)), (FIXED_MULTIPLY(z_r, z_i)))) + c.i;

    z_i = zn_i;
    z_r = zn_r;

    if(z_i_2 + z_r_2 > INFTY_SQR_FIXED) return it;
  }
  return ITERS;
}

int iterate(FixedCord c);

unsigned int mandelbrot_bordertrace(uint16_t *framebuffer, uint16_t *colorscheme, struct camera cam, const struct window win) {
  unsigned int total_iters = 0;
  size_t on_pixel = 0;
  bool border_scanning = false;

  FixedCord scale = { 
    .r = DOUBLE_TO_FIXED((cam.max_r - cam.min_r) / (double)RES_X), 
    .i = DOUBLE_TO_FIXED((cam.max_i - cam.min_i) / (double)RES_Y)
  };
  
  FixedCord c = { 
    .i = DOUBLE_TO_FIXED((((cam.max_i - cam.min_i) * (RES_Y - win.y0)) / RES_Y) + cam.min_i),
    .r = DOUBLE_TO_FIXED((((cam.max_r - cam.min_r) * win.x0) / RES_X) + cam.min_r)
  };


  uint64_t r0 = c.r;
  

  for(int y = win.y0; y < (win.y0 + win.h); y++) {
    border_scanning = false;
    c.r = r0;
    for(int x = win.x0; x < (win.x0 + win.w); x++) {
        if(framebuffer[on_pixel] & G_MASK) { 
          border_scanning = false;
          framebuffer[on_pixel] &= G_MASK;
        }
        else if(border_scanning) { framebuffer[on_pixel] &= G_MASK; }
        else {
          int i = iterate(c);
          total_iters += i;
          framebuffer[on_pixel] = colorscheme[i];
          if(i == ITERS) {
            FixedCord this_coord = c;
            size_t this_index = on_pixel;
            bool seperated_from_start = false;
            bool nei_canidate[8];
            int nei_presort[8];

            size_t backstack[BACKSTACK_SIZE];
            int backstack_i = 0;
            int backstack_calls = 0;

            int nei_dir;


            bool borders[8];
            detect_borders(borders, this_index, win);
            //interior check
            for(nei_dir = 0; nei_dir < 8; nei_dir++) {
              if(borders[nei_dir] || (framebuffer[get_neighbor_index(on_pixel, nei_dir, win)] & GCHAN_EXTERNAL)) break;
            }



            if(nei_dir < 8) {
              while(true) {
                bzero(nei_presort, sizeof(nei_presort));
                bzero(nei_canidate, sizeof(nei_canidate));
                detect_borders(borders, this_index, win);

                //step 1: check pixels around us, fill in neighbors.

                /** now fill in neighbor info based on green channel,
                 *  iterate if no info available.
                 *  if this is to slow we could flatten this; it's predictable
                 *  where there will be info
                 **/  
                bool start_is_nei = false;
                for(int nei_dir = 0; nei_dir < 8; nei_dir++) {
                  size_t nei_i;
                  uint8_t gchan_info;

                  //happens if we're pushed against the screen
                  if(borders[nei_dir]) {
                    nei_presort[nei_dir] = GCHAN_EXTERNAL;
                    continue;
                  }

                  nei_i = get_neighbor_index(this_index, nei_dir, win);
                  gchan_info = framebuffer[nei_i] & G_MASK;
                  if(nei_i == on_pixel) start_is_nei = true;
                  if(gchan_info) nei_presort[nei_dir] = gchan_info;
                  else {
                    int i = iterate(get_neighbor_coord(this_coord, nei_dir, scale));
                    framebuffer[nei_i] = colorscheme[i];
                    nei_presort[nei_dir] = 
                      (i >= ITERS) ? GCHAN_INTERNAL : GCHAN_EXTERNAL;
                    framebuffer[nei_i] |= nei_presort[nei_dir];
                  }
                }
                if(!start_is_nei && !seperated_from_start && (this_index != on_pixel)) seperated_from_start = true;
                if(start_is_nei && seperated_from_start) {
                  framebuffer[this_index] |= GCHAN_BLOCKED;
                  break;
                }

                int edge_cnt = 0;
                //see what neighbors are good canidates for the next pixel in our path
                for(int nei_dir = 0; nei_dir < 8; nei_dir += 2) {
                  int nei_edge_i;
                  if(nei_presort[nei_dir] != GCHAN_INTERNAL) {
                    continue; 
                  }

                  for(nei_edge_i = -2; nei_edge_i <= 2; nei_edge_i++) {
                    int nei_edge_mod = mod((nei_dir + nei_edge_i), 8);
                    if((nei_presort[nei_edge_mod] == GCHAN_EXTERNAL) || borders[nei_edge_mod]) break;
                  }

                  //no edge found
                  if(nei_edge_i > 2) continue;

                  //narrow bridge scenario
                  if(nei_presort[mod((nei_dir + 1), 8)] & nei_presort[mod((nei_dir - 1), 8)] & GCHAN_EXTERNAL) 
                    continue;

                  edge_cnt++;
                  nei_canidate[nei_dir] = true;
                }
                if(edge_cnt >= 2) backstack[backstack_i++ % BACKSTACK_SIZE] = this_index;

                //now go to canidate with lowest prioraty
                framebuffer[this_index] |= GCHAN_BLOCKED;
                for(nei_dir = 0; nei_dir < 8; nei_dir += 2) {
                  if(!nei_canidate[nei_dir]) continue;
                  backstack_calls = 0;
                  this_index = get_neighbor_index(this_index, nei_dir, win);
                  this_coord = get_neighbor_coord(this_coord, nei_dir, scale);
                  break;
                }
                if(nei_dir >= 8) {
                  if((backstack_calls++ > BACKSTACK_SIZE) || (backstack_i < 1)) break;
                  this_index = backstack[--backstack_i % BACKSTACK_SIZE];
                  this_coord.r = DOUBLE_TO_FIXED(((((this_index % win.w) + win.x0) / (double)RES_X) * (cam.max_r - cam.min_r)) + cam.min_r);
                  this_coord.i = DOUBLE_TO_FIXED(((((this_index / (double)win.w) + win.y0) / (double)RES_Y) * (cam.min_i - cam.max_i)) + cam.max_i);
                }
              }
            }
            else border_scanning = true;
          }
          else framebuffer[on_pixel] |= GCHAN_EXTERNAL;
        }
      on_pixel++;
      c.r += scale.r;
    }
    c.i -= scale.i;
  }
  for(size_t i = 0; i < (win.w * win.h); i++) framebuffer[i] &= ~G_MASK;
  return total_iters;
}

//TODO rename
unsigned int render_mandelbrot(uint16_t *framebuffer, uint16_t *colorscheme, struct camera cam, int x0, int y0, int w, int h) {
  int32_t scale_i = DOUBLE_TO_FIXED((cam.max_i - cam.min_i) / (double)RES_Y);
  int32_t scale_r = DOUBLE_TO_FIXED((cam.max_r - cam.min_r) / (double)RES_X);
  int32_t c_i = DOUBLE_TO_FIXED((((cam.max_i - cam.min_i) * (RES_Y - y0)) / RES_Y) + cam.min_i);
  int32_t c_r0 = DOUBLE_TO_FIXED((((cam.max_r - cam.min_r) * x0) / RES_X) + cam.min_r);
  int32_t c_r, z_i, z_r, zn_r, z_r_2, z_i_2;
  size_t fb_index = 0;
  int i; 
  unsigned int total_iters = 0;

  for(int y = y0; y < y0 + h; y++) {
    c_r = c_r0;
    for(int x = x0; x < x0 + w; x++) {
      z_i = 0;
      z_r = 0;
      for(i = 0; i < ITERS; i++) {
        z_r_2 = FIXED_MULTIPLY(z_r, z_r);
        z_i_2 = FIXED_MULTIPLY(z_i, z_i);

        zn_r = z_r_2 - z_i_2 + c_r;

        z_i = (FIXED_MULTIPLY(z_r, z_i) << 1)  + c_i;

        z_r = zn_r;

        if(z_i_2 + z_r_2 > INFTY_SQR_FIXED) break;
      }
      total_iters += i;
      framebuffer[fb_index++] = colorscheme[i];
      c_r += scale_r;
    }
    c_i -= scale_i;
  }
  return total_iters;
}
  


#define FB_SIZE_X RES_X/2
#define FB_SIZE_Y RES_Y

//TODO rename
void draw_mandelbrot() {
  uint16_t framebuffer[FB_SIZE_X * FB_SIZE_Y];
  uint16_t columnbuffer[(size_t)(STEP_SIZE * RES_X * RES_Y)];
  uint16_t left_line = 0;
  //program flow is awful atm becuase I was planning something different; will be improved soon.
  /**
  static struct camera cam = {
    .min_r = CAM_DEF_MIN_R,
    .max_r = CAM_DEF_MAX_R,
    .min_i = ((double)RES_Y / RES_X) * CAM_DEF_MIN_R,
    .max_i = ((double)RES_Y / RES_X) * CAM_DEF_MAX_R,
  };
  **/
  
  struct camera cam = {
    .min_r = 1.511138965827779623297, .min_i = -0.000099833545397436595, .max_r = 1.513299500557251375810, .max_i = 0.000980433819429512915
  };
  uint16_t colorscheme[ITERS + 1];


  
  /** yes, I know the following is disgusting. Before I clean it, I just wanna get the general idea out, 
    it's more efficient in that order
    TODO once you  get your idea ironed out, clean code **/
  
  init_colorscheme(colorscheme);
  bzero(framebuffer, sizeof(framebuffer));
  bzero(columnbuffer, sizeof(columnbuffer));

  while(true) {
    const int y_offset = STEP_SIZE * FB_SIZE_Y;
    const int x_offset = STEP_SIZE * RES_X;
    const size_t top_space = FB_SIZE_X * (FB_SIZE_Y - y_offset) * sizeof(uint16_t);
    left_line = left_line ? (RES_X/2) : 0; 
    switch(event_get()) {
      case BUTTON_UP:
        cam_shift(&cam, 0, STEP_SIZE);
        memmove(framebuffer + (FB_SIZE_X * y_offset), framebuffer, top_space);
        mandelbrot_bordertrace(framebuffer, colorscheme, cam, (struct window){left_line, 0, FB_SIZE_X, y_offset});
        break;
      case BUTTON_DOWN:
        cam_shift(&cam, 0, -STEP_SIZE);
        memmove(framebuffer, framebuffer + (FB_SIZE_X * y_offset), top_space);
        mandelbrot_bordertrace(framebuffer + (FB_SIZE_X * (FB_SIZE_Y - y_offset)), colorscheme, cam, (struct window){left_line, (RES_Y - y_offset), FB_SIZE_X, y_offset});
        break;
      case BUTTON_RIGHT:
        cam_shift(&cam, STEP_SIZE, 0);
        mandelbrot_bordertrace(columnbuffer, colorscheme, cam, (struct window){left_line + (FB_SIZE_X - x_offset), 0, x_offset, FB_SIZE_Y});
        for(uint16_t y = 0; y < FB_SIZE_Y; y++) {
          memmove(framebuffer + (FB_SIZE_X * y), framebuffer + (FB_SIZE_X * y) + x_offset, (FB_SIZE_X - x_offset) * sizeof(*framebuffer));
          memmove(framebuffer + (FB_SIZE_X * y) + (FB_SIZE_X - x_offset), columnbuffer + (x_offset * y), x_offset * sizeof(*framebuffer));
        }
        break;
      case BUTTON_LEFT:
        cam_shift(&cam, -STEP_SIZE, 0);
        mandelbrot_bordertrace(columnbuffer, colorscheme, cam, (struct window){left_line, 0, x_offset, FB_SIZE_Y});
        for(uint16_t y = 0; y < FB_SIZE_Y; y++) {
          memmove(framebuffer + (FB_SIZE_X * y) + x_offset, framebuffer + (FB_SIZE_X * y), (FB_SIZE_X - x_offset) * sizeof(*framebuffer));
          memmove(framebuffer + (FB_SIZE_X * y), columnbuffer + (x_offset * y), x_offset * sizeof(*framebuffer));
        }
        break;
      case BUTTON_A:
        cam_zoom(&cam, ZOOM_SIZE);
        mandelbrot_bordertrace(framebuffer, colorscheme, cam, (struct window){left_line, 0, FB_SIZE_X, FB_SIZE_Y});
        break;
      case BUTTON_B:
        cam_zoom(&cam, -ZOOM_SIZE);
        mandelbrot_bordertrace(framebuffer, colorscheme, cam, (struct window){left_line, 0, FB_SIZE_X, FB_SIZE_Y});
        break;
      default:
        mandelbrot_bordertrace(framebuffer, colorscheme, cam, (struct window){left_line, 0, FB_SIZE_X, FB_SIZE_Y});
    }
    ST7735_DrawImage(left_line, 0, FB_SIZE_X, FB_SIZE_Y, framebuffer);

    left_line = left_line ? 0 : FB_SIZE_X; 
    mandelbrot_bordertrace(framebuffer, colorscheme, cam, (struct window){left_line, 0, FB_SIZE_X, FB_SIZE_Y});
    ST7735_DrawImage(left_line, 0, FB_SIZE_X, FB_SIZE_Y, framebuffer);
    idle();
  }
}
