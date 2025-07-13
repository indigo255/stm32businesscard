#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <raylib.h>
#include <raymath.h>
#include <limits.h>
#include <complex.h>
#include <string.h>

#define WINDOW_SIZE_X     1600
#define WINDOW_SIZE_Y     800
#define RES_X             1600
#define RES_Y             800
#define DEFAULT_CENTER_X  0
#define DEFAULT_CENTER_Y  0
#define MOUSE_BUTTON      0
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

//#define SHIP
//#undef SHIP

#ifdef SHIP
Color get_color(int i) {
  if(i == ITERS) return (Color){0, 0, 0, 255};
  if(i == 0) return (Color){0, 0, 0, 255};
    return (Color) {
        2*(i - 128)+255,
          0,
          0,
        255
    };
}
#else
Color get_color(int i) {
//  if((i == ITERS) || (i == 0)) return (Color){0, 0, 0, 255};
  if(i == ITERS) return (Color){0,0,0,255};
  if(i == 0) return (Color){0,0,1,255};
  if(i < 128) {
    return (Color) {
        (8*(i - 128)+255) & 0xff,
        0,
        (16*(i - 64)+255) & 0xff,
        255
    };
  }
    return (Color) {
      0,
        0,
      ((unsigned int)-2*(i - 128)+255) & 0xff,
        255
    };
}
#endif


struct camera {
  double min_r, min_i, max_r, max_i;
};

typedef struct {
  int32_t r; int32_t i;
} FixedCord;

static inline int iterate(FixedCord c) {
  int32_t z_i = 0;
  int32_t z_r = 0;
  int32_t z_r_2, z_i_2, zn_r, zn_i;
  for(int it = 0; it < ITERS; it++) {
    z_r_2 = FIXED_MULTIPLY(z_r, z_r);
    z_i_2 = FIXED_MULTIPLY(z_i, z_i);

    zn_r = z_r_2 - z_i_2 + c.r;

#ifdef SHIP
    zn_i = abs(FIXED_MULTIPLY((DOUBLE_TO_FIXED(2)), (FIXED_MULTIPLY(z_r, z_i)))) + c.i;
#else
    zn_i = (FIXED_MULTIPLY((DOUBLE_TO_FIXED(2)), (FIXED_MULTIPLY(z_r, z_i)))) + c.i;
#endif

    z_i = zn_i;
    z_r = zn_r;

    if(z_i_2 + z_r_2 > INFTY_SQR_FIXED) return it;
  }
  return ITERS;
}

//blllluuuuurg, matracies and vectors in raylib are floats and we need doubles
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

enum DIRECTIONS {
  N, NE, E, SE, S, SW, W, NW
};


//we can inline these if needed

inline bool bitarray_check(uint8_t *array, size_t i) {
  return array[i/8] & (1 << (i%8));
}

inline void bitarray_set(uint8_t *array, size_t i) {
  array[i/8] |= (1 << (i%8));
}

inline FixedCord get_neighbor_coord(FixedCord from_coord, int direction, FixedCord step) {
  if((direction == NW) || (direction < E)) from_coord.i += step.i;
  if((direction > N) && (direction < S)) from_coord.r += step.r;
  if((direction > E) && (direction < W)) from_coord.i -= step.i;
  if(direction > S) from_coord.r -= step.r;
  return from_coord;
}
FixedCord get_neighbor_coord(FixedCord from_coord, int direction, FixedCord step);


size_t get_neighbor_index(size_t from_pixel, int direction) {
  const size_t neighbor_index_accl[8] = 
    {-RES_X, -RES_X + 1, 1, RES_X + 1, RES_X, RES_X - 1, -1, -RES_X - 1};
  from_pixel += neighbor_index_accl[direction];
  //canidate for optimization; lots of branches. maybe inline
  return from_pixel;
}



bool bitarray_check(uint8_t *array, size_t i);
void bitarray_set(uint8_t *array, size_t i);
#define BITARRAY_SET(array, i) ((array)[(i)/8] |= (1 << ((i) % 8)))
#define BITARRAY_CLEAR(array, i) ((array)[(i)/8] &= ~(1 << ((i) % 8)))
#define BITARRAY_CHECK(array, i) ((array)[(i)/8] & (1 << ((i) % 8)))
          

#define PRESORT_UNSOLVED      (1 << 0)
#define PRESORT_VISITED       (1 << VISITED)  
//#define PRESORT_INTERIOR      (1 << INTERIOR) //in set, NOT an edge
//#define PRESORT_BACKTRACKED   (1 << BACKTRACED)
//#define PRESORT_EXTERIOR      (1 << 

//we'll be storing info in the green channel to utalize all available memory.
//the following is bitmasks to do so
#define GCHAN_RENDERED      (1 << 0)
#define GCHAN_VISITED       (1 << 1)  
#define GCHAN_BLOCKED       (1 << 2) //interior element or backtrack
#define GCHAN_DEBUG         (1 << 3) //extra, not nessesary

/**
void switch_pixel(coord &this_coord, const coord step, size_t this_index, int dir) {

}
**/

unsigned int mandelbrot_bordertrace(struct camera *cam, Color *pixels) {
  //these lookup tables r cheap cuz on the stm32f1, 1 memory read is 1 instruction
  FixedCord scale = { 
    .r = DOUBLE_TO_FIXED((cam->max_r - cam->min_r) / (double)RES_X), 
    .i = DOUBLE_TO_FIXED((cam->max_i - cam->min_i) / (double)RES_Y)};
  FixedCord c = {.r = 0, .i = DOUBLE_TO_FIXED(cam->max_i)};
  unsigned int total_iters = 0;
  size_t on_pixel = 0;

  //having these r kinda gross, will restructure later
  int32_t cam_bord_fixed_n = DOUBLE_TO_FIXED(cam->min_i);
  int32_t cam_bord_fixed_s = DOUBLE_TO_FIXED(cam->max_i);
  int32_t cam_bord_fixed_e = DOUBLE_TO_FIXED(cam->max_r);
  int32_t cam_bord_fixed_w = DOUBLE_TO_FIXED(cam->min_r);


  /**
  //for keeping track of border only. will organize later
  uint8_t set[(160*80)/8] = {0};
  uint8_t unset[(160*80)/8] = {0};
  **/
  for(int y = 0; y < RES_Y; y++) {
    c.r = DOUBLE_TO_FIXED(cam->min_r);
    for(int x = 0; x < RES_X; x++) {
      int i = iterate(c);
      total_iters += i;
      pixels[on_pixel] = get_color(i);

      /**
      const Color debug_colors[] = 
      { (Color) {0xff, 0x00, 0x00, 0xff},
        (Color) {0xff, 0x00, 0xff, 0xff},
        (Color) {0x00, 0xff, 0x00, 0xff},
        (Color) {0x00, 0x00, 0xff, 0xff},
        (Color) {0x6a, 0x00, 0xff, 0xff}
      };

      static int on_dbg_color = 0;
      **/

      if(i == ITERS) {
        FixedCord this_coord = c;
        size_t this_index = on_pixel;
        int nei_priority[8];
        int last_nei_priority[8];
        //only really need to zero green channel
        bzero(pixels, RES_X * RES_Y * sizeof(*pixels)); 
        int last_direction = W;
        int nei_presort[8] = {0};

        //this is so fucking knarly
        while(true) {
          //step 1: check pixels around us, fill in neighbors.

          //find if we're pushed against screen border. 
          //feels gross but I don't think there's a better way
          if((this_coord.i - scale.i) < cam_bord_fixed_n) {
            for(int nei_dir = SE; nei_dir <= SW; nei_dir++)
              nei_presort[nei_dir] = GCHAN_BLOCKED;
          }
          else if((this_coord.i + scale.i) > cam_bord_fixed_s) {
              nei_presort[NE] = GCHAN_BLOCKED;
              nei_presort[N] = GCHAN_BLOCKED;
              nei_presort[NW] = GCHAN_BLOCKED;
          }
          if((this_coord.r - scale.r) < cam_bord_fixed_w) {
            for(int nei_dir = SW; nei_dir < NW; nei_dir++)
              nei_presort[nei_dir] = GCHAN_BLOCKED;
          }
          else if((this_coord.r + scale.r) > cam_bord_fixed_e) {
            for(int nei_dir = NE; nei_dir < SE; nei_dir++)
              nei_presort[nei_dir] = GCHAN_BLOCKED;
          }


          /** now fill in neighbor info based on green channel,
           *  iterate if no info available.
           *  if this is to slow we could flatten this; it's predictable
           *  where there will be info
           **/  
          //if this is too slow, get rid of all the modulos. 
          bool interior = true;
          for(int nei_dir = 0; nei_dir < 8; nei_dir++) {
            size_t nei_i;
            uint8_t gchan_info;
            nei_i = get_neighbor_index(this_index, nei_dir);
            gchan_info = pixels[this_index].g;
            //note that when we move this over, there will be no alpha channel.
            //gchan_info will be extracted differently!!!
            if(gchan_info & GCHAN_RENDERED) {
              gchan_info &= ~(GCHAN_RENDERED);
              gchan_info ^= GCHAN_BLOCKED << 1;
              nei_presort[nei_i] = gchan_info;
              if(!gchan_info) interior = false;
              /**
              if(gchan_info & GCHAN_BLOCKED) nei_presort[nei_dir] = -1;
              else if(gchan_info & GCHAN_VISITED) nei_priority[nei_dir] = 12;
              else if((pixels[nei_i].r | pixels[nei_i].b) == 0) //unvisited element
                nei_priority[nei_dir] = 9;
              else { //exterior
                interior = false;
                nei_priority[nei_dir] = -1;
              }
              **/
            }
            else {
              int i = iterate(get_neighbor_coord(this_coord, nei_dir, scale));
              pixels[nei_i] = get_color(i);
              pixels[nei_i].g |= GCHAN_RENDERED;
              if(i == ITERS) continue;
              interior = false;
            }
            //TODO if interior true
            /**
            if((nei_priority[nei_dir] == 6) && (nei_i > 0)) {
              if(last_nei_unvisited) {
                nei_priority[nei_dir] = 3;
                nei_priority[nei_dir-1] = 3;
              }
              else if(nei_priority[nei_dir-1] == 12)
                nei_priority[nei_dir-1] = 6;
              LAST_nei_unvisited = true;
            }
            else last_nei_unvisited = false;
            **/
          }
          if(interior) {
            memcpy(last_nei_priority, nei_priority, sizeof(nei_priority));
            pixels[this_index].g |= GCHAN_BLOCKED;
            this_index = get_neighbor_index(this_index, (last_direction + 4) % 8);
            this_coord = get_neighbor_coord(
                this_coord, (last_direction + 4) % 8, scale);

          }
          else {
            for(int nei_dir = 0; nei_dir < 8; nei_dir++) {
              int offset_cw = (last_direction - nei_dir) % 8;
              int offset_ccw = (nei_dir - last_direction) % 8;
              int forward_offset;
              switch(nei_presort[nei_dir]) {
                case GCHAN_VISITED:
                  nei_priority[nei_dir] = 12;
                  break;
                case GCHAN_BLOCKED:
                  nei_priority[nei_dir] = -1;
                  break;
                default: //unvisited element
                  if((nei_presort[(nei_dir - 1) % 8] == 0) || 
                      (nei_presort[(nei_dir + 1) % 8] == 0)) {
                    nei_priority[nei_dir] = 3;
                    break;
                  }
                  if((nei_presort[(nei_dir - 1) % 8] == GCHAN_VISITED) || 
                      (nei_presort[(nei_dir + 1) % 8] == GCHAN_VISITED)) {
                    nei_priority[nei_dir] = 6;
                    break;
                  }
                  nei_priority[nei_dir] = 9;
                  break;
              }
              forward_offset = (offset_cw < offset_ccw) ? offset_cw : offset_ccw;
              if(forward_offset < 3) nei_priority[nei_dir] -= (3-forward_offset);
            }
          }
          for(int priority = 0; priority < 12; priority++) {
            for(int nei_dir = 0; nei_dir < 8; nei_dir++) {
              if(nei_priority[nei_dir] != priority) continue;
              //switch pixels!!!!1!!
              memcpy(last_nei_priority, nei_priority, sizeof(nei_priority));
              last_direction = nei_dir;
              this_index = get_neighbor_index(this_index, nei_dir);
              this_coord = get_neighbor_coord(this_coord, nei_dir, scale);
              
              goto drunkaloneandunemployed;
            }
          }
          drunkaloneandunemployed:
        }
      }
      on_pixel++;
      c.r += scale.r;
    }
    c.i -= scale.i;
  }
  return total_iters;
}

unsigned int mandelbrot_unoptimized(struct camera *cam, Color *pixels) {
  FixedCord scale = { .r = DOUBLE_TO_FIXED((cam->max_r - cam->min_r) / (double)RES_X), .i = DOUBLE_TO_FIXED((cam->max_i - cam->min_i) / (double)RES_Y) };
  FixedCord c = { .r = 0, .i = DOUBLE_TO_FIXED(cam->max_i) };
  unsigned int total_iters = 0;
  for(int y = 0; y < RES_Y; y++) {
    c.r = DOUBLE_TO_FIXED(cam->min_r);
    for(int x = 0; x < RES_X; x++) {
      int i = iterate(c);
      total_iters += i;
      pixels[((y * RES_X) + x)] = get_color(i);
      c.r += scale.r;
    }
    c.i -= scale.i;
  }
  return total_iters;
}

void test() {
  uint8_t bitarray[(160*80)/8] = {0};
  int test_i = 9;
  BITARRAY_SET(bitarray, test_i);
  printf("%s\n", BITARRAY_CHECK(bitarray, 9) ? "true" : "false");
}

int main() {
  //test();
  //return 0;
  Color *pixels = malloc(RES_X * RES_Y * sizeof(Color));
  struct camera cam = {
    .min_r = -1,
    .max_r = 1,
  //  .min_i = -1,
  //  .max_i = 1
  };
  cam.min_i = ((double)RES_Y / RES_X) * cam.min_r;
  cam.max_i = ((double)RES_Y / RES_X) * cam.max_r;
  InitWindow(WINDOW_SIZE_X, WINDOW_SIZE_Y, "mandelbrot fixed point test");

  Image img = GenImageColor(RES_X, RES_Y, BLUE);
  Texture tex = LoadTextureFromImage(img);
  UnloadImage(img);

  SetTargetFPS(0);


  while(!WindowShouldClose()) {
    switch(GetKeyPressed()) {
      case KEY_UP:
        shift_cam(&cam, 0, STEP_SIZE);
        break;
      case KEY_DOWN:
        shift_cam(&cam, 0, -STEP_SIZE);
        break;
      case KEY_RIGHT:
        shift_cam(&cam, STEP_SIZE, 0);
        break;
      case KEY_LEFT:
        shift_cam(&cam, -STEP_SIZE, 0);
        break;
      case KEY_W:
        zoom_cam(&cam, ZOOM_SIZE);
        break;
      case KEY_S:
        zoom_cam(&cam, -ZOOM_SIZE);
        break;
      default:
        BeginDrawing();
        EndDrawing();
        continue;
        break;
    }
    printf("(%.21f, %.21f) - (%.21f, %.21f)\n", cam.min_r, cam.min_i, cam.max_r, cam.max_i);

    printf("Unoptimized: %u iterations\n", mandelbrot_unoptimized(&cam, pixels));
    printf("Border tracing: %u iterations\n", mandelbrot_bordertrace(&cam, pixels));

    BeginDrawing();
    UpdateTexture(tex, pixels);
    DrawTextureEx(tex, (Vector2){0,0}, 0, (float)GetRenderWidth()/RES_X, WHITE);
    EndDrawing();
  }

  return 0;
}
