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
          
//a lot of these are just so I can keep track of my cases while I program this, simplification will happen later
enum CANIDATE_STATUS {
  UNSOLVED = 0,
  M_ELEMENT,
  M_EXTERIOR,
  M_INTERIOR,
  M_VISITED, //part of the curve we've been drawing
  M_SKETCHY_SUSPENSION_ROPE_BRIDGE_TYPE_SHIT
};

unsigned int mandelbrot_bordertrace(struct camera *cam, Color *pixels) {
  //these lookup tables r cheap cuz on the stm32f1, 1 memory read is 1 instruction
  FixedCord scale = { 
    .r = DOUBLE_TO_FIXED((cam->max_r - cam->min_r) / (double)RES_X), 
    .i = DOUBLE_TO_FIXED((cam->max_i - cam->min_i) / (double)RES_Y)};
  FixedCord c = {.r = 0, .i = DOUBLE_TO_FIXED(cam->max_i)};
  unsigned int total_iters = 0;
  size_t on_pixel = 0;
  uint8_t border[(RES_X*RES_Y)/8] = {0};

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
      uint8_t border_bit = BITARRAY_CHECK(border, on_pixel);
      int i = iterate(c);
      total_iters += i;
      pixels[on_pixel] = get_color(i);

      const Color debug_colors[] = 
      { (Color) {0xff, 0x00, 0x00, 0xff},
        (Color) {0xff, 0x00, 0xff, 0xff},
        (Color) {0x00, 0xff, 0x00, 0xff},
        (Color) {0x00, 0x00, 0xff, 0xff},
        (Color) {0x6a, 0x00, 0xff, 0xff}
      };

      static int on_dbg_color = 0;

      //this is where it all begins
      uint8_t rendered[(RES_X*RES_Y)/8] = {0};
      uint8_t deadend[(RES_X*RES_Y)/8] = {0};
      if(i == ITERS) {
        BORDER_START:
        __attribute__((unused)); 

        //just makes getting index of neigbor easier; doesn't cost extra cycles
        FixedCord starting_bord_cord = c;
        FixedCord current_bord_cord = c;
        FixedCord last_bord_cord;
        int previous_neighbors[8] = {UNSOLVED};
        int current_neighbors[8] = {UNSOLVED};
        current_neighbors[W] = UNSOLVED;
        size_t current_bord_i = on_pixel;
        uint8_t visited_border[(RES_X*RES_Y)/8] = {0};
        int source_dir = 0;
        size_t prev_bord_i = 0;
        while(true) {
          int filled_neighbors = 0;

          //find if we're pushed against screen border. 
          //find a less gross way to do this
          if((current_bord_cord.i - scale.i) < cam_bord_fixed_n) {
            for(int nei_dir = SE; nei_dir <= SW; nei_dir++)
              current_neighbors[nei_dir] = M_EXTERIOR;
          }
          if((current_bord_cord.i + scale.i) > cam_bord_fixed_s) {
              current_neighbors[NE] = M_EXTERIOR;
              current_neighbors[N] = M_EXTERIOR;
              current_neighbors[NW] = M_EXTERIOR;
          }
          if((current_bord_cord.r - scale.r) < cam_bord_fixed_w) {
            for(int nei_dir = SW; nei_dir < NW; nei_dir++)
              current_neighbors[nei_dir] = M_EXTERIOR;
          }
          if((current_bord_cord.r + scale.r) > cam_bord_fixed_e) {
            for(int nei_dir = NE; nei_dir < SE; nei_dir++)
              current_neighbors[nei_dir] = M_EXTERIOR;
          }

          //get info on neighbors, fill in missing current_neighbors info
          for(int nei_dir = 0; nei_dir < 8; nei_dir++) {
            size_t nei_i = get_neighbor_index(current_bord_i, nei_dir);
            if(current_neighbors[nei_dir] == M_EXTERIOR) continue;
            if(current_neighbors[nei_dir] != UNSOLVED) {
              filled_neighbors++;
              continue;
            }
            if(current_neighbors[nei_dir] == M_VISITED) continue;
            if(BITARRAY_CHECK(visited_border, nei_i)) {
              current_neighbors[nei_dir] = M_VISITED;
              filled_neighbors++;
              continue;
            }
            int i = iterate(get_neighbor_coord(current_bord_cord, nei_dir, scale));
            total_iters += i;
            if(i == ITERS) {
              current_neighbors[nei_dir] = M_ELEMENT;
              filled_neighbors++;
              continue;
            }
            current_neighbors[nei_dir] = M_EXTERIOR;
          }

          if(filled_neighbors >= 8) {
            memcpy(current_neighbors, previous_neighbors, sizeof(current_neighbors));
            current_bord_cord = last_bord_cord;
            current_bord_i = prev_bord_i;
            current_neighbors[source_dir] = M_INTERIOR;
          }

          BeginDrawing();
          DrawPixel(current_bord_i % RES_X, current_bord_i / RES_X, debug_colors[on_dbg_color]);
          EndDrawing();

          int nei_dir;
          memcpy(previous_neighbors, current_neighbors, sizeof(current_neighbors));
          memset(current_neighbors, 0, sizeof(current_neighbors));
          for(nei_dir = 0; nei_dir < 8; nei_dir++) {
            //found a valid neighbor to switch to
            if(previous_neighbors[nei_dir] == M_ELEMENT) { 
              BITARRAY_SET(visited_border, current_bord_i);
              current_neighbors[(nei_dir + 4) % 8] = M_VISITED;
              if(nei_dir % 2) { //diagnals
                current_neighbors[(nei_dir + 3) % 8] = previous_neighbors[(nei_dir + 1) % 8]; 
                current_neighbors[(nei_dir + 5) % 8] = previous_neighbors[(nei_dir - 1) % 8]; 
              }
              else {
                current_neighbors[(nei_dir + 2) % 8] = previous_neighbors[(nei_dir + 1) % 8];
                current_neighbors[(nei_dir + 3) % 8] = previous_neighbors[(nei_dir + 2) % 8];
                current_neighbors[(nei_dir + 5) % 8] = previous_neighbors[(nei_dir - 2) % 8];
                current_neighbors[(nei_dir + 6) % 8] = previous_neighbors[(nei_dir - 1) % 8];
              }

              last_bord_cord = current_bord_cord;
              current_bord_cord = get_neighbor_coord(current_bord_cord, nei_dir, scale);

              prev_bord_i = current_bord_i;
              current_bord_i = get_neighbor_index(current_bord_i, nei_dir);

              
              source_dir = nei_dir;
              break;
            }
          }
          if(!memcmp(&current_bord_cord, &starting_bord_cord, sizeof(current_bord_cord))) {
            for(size_t bord_i = 0; bord_i < sizeof(border); bord_i++){
              border[bord_i] |= visited_border[bord_i];
              break;
            }
          }
          printf("%zu: ", prev_bord_i);
          printf("(%zu, %zu) -> (%zu, %zu) | {", prev_bord_i % RES_X, prev_bord_i / RES_X, current_bord_i % RES_X, current_bord_i / RES_X);
          for(int i = 0; i < 8; i++) printf("%i, ", previous_neighbors[i]);
          printf("} -> {");
          for(int i = 0; i < 8; i++) printf("%i, ", current_neighbors[i]);
          printf("}\n");

          if(nei_dir > 7) break;
          //printf("loop\n");
        } 
        on_dbg_color = (on_dbg_color + 1) % (sizeof(debug_colors) / sizeof(*debug_colors));
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
