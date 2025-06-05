#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <raylib.h>
#include <raymath.h>
#include <limits.h>
#include <complex.h>

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

#define SHIP
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
  if(i == ITERS) return (Color){0,255,0,255};
  if(i == 0) return (Color){0, 0, 0, 255};
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

struct vec2_double {
  double x, y;
};

struct vec2_float {
  int32_t x, y;
};

static inline int iterate(int32_t r, int32_t i) {
  int32_t z_i = 0;
  int32_t z_r = 0;
  int32_t z_r_2, z_i_2, zn_r, zn_i;
  for(int it = 0; it < ITERS; it++) {
    z_r_2 = FIXED_MULTIPLY(z_r, z_r);
    z_i_2 = FIXED_MULTIPLY(z_i, z_i);

    zn_r = z_r_2 - z_i_2 + r;

#ifdef SHIP
    zn_i = abs(FIXED_MULTIPLY((DOUBLE_TO_FIXED(2)), (FIXED_MULTIPLY(z_r, z_i)))) + i;
#else
    zn_i = (FIXED_MULTIPLY((DOUBLE_TO_FIXED(2)), (FIXED_MULTIPLY(z_r, z_i)))) + i;
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

typedef struct {
  double x; double y;
} coordinate;

//we can inline these if needed

inline bool bitarray_check(uint8_t *array, size_t i) {
  return array[i/8] & (1 << (i%8));
}

inline void bitarray_set(uint8_t *array, size_t i) {
  array[i/8] |= (1 << (i%8));
}

/**
inline coordinate get_neighbor_coord(coordinate from_coord, int direction, coordinate step) {
  if((direction == NW) && (direction < E)) from_coord.x += ;
  if((direction > N) && (direction < S)) from_coord += 1;
  if((direction > E) && (direction < W)) from_coord += RES_X;
  if(direction > S) from_coord -= 1;
  return from_coord;
}
**/


size_t get_neighbor_index(size_t from_pixel, int direction) {
  //canidate for optimization; lots of branches. maybe inline
  if((direction == NW) && (direction < E)) from_pixel -= RES_X;
  if((direction > N) && (direction < S)) from_pixel += 1;
  if((direction > E) && (direction < W)) from_pixel += RES_X;
  if(direction > S) from_pixel -= 1;
  return from_pixel;
}


bool bitarray_check(uint8_t *array, size_t i);
void bitarray_set(uint8_t *array, size_t i);
#define BITARRAY_SET(array, i) ((array)[(i)/8] |= (1 << ((i) % 8)))
#define BITARRAY_CHECK(array, i) ((array)[(i)/8] & (1 << ((i) % 8)))
          
/**
          enum CANIDATE_STATUS {
            UNSOLVED = 0,
            CANIDATE,
            NONCANIDATE
          };
          **/

unsigned int mandelbrot_bordertrace(struct camera *cam, Color *pixels) {
  //these lookup tables r cheap cuz on the stm32f1, 1 memory read is 1 instruction
  const size_t neighbor_index_accl[8] = {-RES_X, -RES_X + 1, 1, RES_X + 1, RES_X, RES_X - 1, -1, -RES_X - 1};
  int32_t scale_i = DOUBLE_TO_FIXED((cam->max_i - cam->min_i) / (double)RES_Y);
  int32_t scale_r = DOUBLE_TO_FIXED((cam->max_r - cam->min_r) / (double)RES_X);
  int32_t c_i = DOUBLE_TO_FIXED(cam->max_i);
  int32_t c_r;
  unsigned int total_iters = 0;
  size_t on_pixel = 0;
  uint8_t border[(160*80)/8] = {0};
  //for keeping track of border only. will organize later
  uint8_t set[(160*80)/8] = {0};
  uint8_t unset[(160*80)/8] = {0};
  for(int y = 0; y < RES_Y; y++) {
    c_r = DOUBLE_TO_FIXED(cam->min_r);
    for(int x = 0; x < RES_X; x++) {
      int i = iterate(c_r, c_i);
      total_iters += i;
      pixels[((y * RES_X) + x)] = get_color(i);

      //this is where it all begins
      if(i == ITERS) {
        int current_border_pixel = on_pixel;
        uint8_t visited_border[(160*80)/8] = {0};
        int filled_neighbors = 0;
        //unroll and manually get_neighbor if this is too slow
        for(int nei_dir = 0; nei_dir < 8; nei_dir++) {
          size_t nei_i = current_border_pixel + neighbor_index_accl[nei_dir];
          if(bitarray_check(set, nei_i)) {
            filled_neighbors++;
            continue;
          }
          if(bitarray_check(unset, nei_i)) continue;
          
        }
        //if c_d == 7 go back
      }
      c_r += scale_r;
    }
    on_pixel++;
    c_i -= scale_i;
  }
  return total_iters;
}

unsigned int mandelbrot_unoptimized(struct camera *cam, Color *pixels) {
  int32_t scale_i = DOUBLE_TO_FIXED((cam->max_i - cam->min_i) / (double)RES_Y);
  int32_t scale_r = DOUBLE_TO_FIXED((cam->max_r - cam->min_r) / (double)RES_X);
  int32_t c_i = DOUBLE_TO_FIXED(cam->max_i);
  int32_t c_r;
  unsigned int total_iters = 0;
  for(int y = 0; y < RES_Y; y++) {
    c_r = DOUBLE_TO_FIXED(cam->min_r);
    for(int x = 0; x < RES_X; x++) {
      int i = iterate(c_r, c_i);
      total_iters += i;
      pixels[((y * RES_X) + x)] = get_color(i);
      c_r += scale_r;
    }
    c_i -= scale_i;
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
  test();
  return 0;
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

  SetTargetFPS(10);


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
    //printf("Border tracing: %u iterations\n", mandelbrot_unoptimized(cam, pixels);

    BeginDrawing();
    UpdateTexture(tex, pixels);
    DrawTextureEx(tex, (Vector2){0,0}, 0, (float)GetRenderWidth()/RES_X, WHITE);
    EndDrawing();
  }

  return 0;
}
