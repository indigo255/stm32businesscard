/** READ BEFORE JUDING!
 * Yes, I know this code is a mess. Debug code is added
 * happhazardly, two cameras are used, etc.
 * That's because it's a temporary program
 * to create optimizations and debug rendering issues without hardware.
 * None of this is going to be included in the project, and the code is thus
 * not extensible or organized; it really doesn't save any effort to do so.
 *
 * This code is meant for my eyes only. You've been warned!
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <raylib.h>
#include <raymath.h>
#include <limits.h>
#include <complex.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define WINDOW_SIZE_X     1600
#define WINDOW_SIZE_Y     800
#define RES_X             160
#define RES_Y             80
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
//#define COLOR_DEBUG
//#define SHIP

Color get_color_dbg(int i) {
  if(i == ITERS) return (Color){0,0,255,255};
//  if(i == 0) return (Color){255,255,255,255};
  return (Color){255,0,255,255};
}
  

#ifdef COLOR_DEBUG
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

//C does remainder, not modulo.
//TODO optimize for mod 8. Benchmark
inline int mod(int n, int d) {
  int r = n % d;
  return (r < 0) ? r + d : r;
}
int mod(int n, int d);


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

inline FixedCord get_neighbor_coord(FixedCord from_coord, int direction, FixedCord step) {
  if((direction == NW) || (direction < E)) from_coord.i += step.i;
  if((direction > N) && (direction < S)) from_coord.r += step.r;
  if((direction > E) && (direction < W)) from_coord.i -= step.i;
  if(direction > S) from_coord.r -= step.r;
  return from_coord;
}
FixedCord get_neighbor_coord(FixedCord from_coord, int direction, FixedCord step);


size_t get_neighbor_index(size_t from_pixel, int direction) {
  const int neighbor_index_accl[8] = 
    {-RES_X, -RES_X + 1, 1, RES_X + 1, RES_X, RES_X - 1, -1, -RES_X - 1};
  from_pixel += neighbor_index_accl[direction];
  //canidate for optimization; lots of branches. maybe inline
  return from_pixel;
}

//we'll be storing info in the green channel to utalize available memory 
//per pixel.

#define BACKSTACK_SIZE      32
#define GCHAN_UNRENDERED    0 //don't change; green channel zero'd on cam move
#define GCHAN_BLOCKED       (1 << 7) //interior element or visiteed
#define GCHAN_INTERNAL      (1 << 5) //part of set, 0x20
#define GCHAN_EXTERNAL      (1 << 0) //not part of set, 0x10
#define GCHAN_INNER_VISITED (1 << 3)
#define GCHAN_INNER_CLOSED  (1 << 2)


/**
void switch_pixel(coord &this_coord, const coord step, size_t this_index, int dir) {

}
**/

void debug_step(Color *pix, Texture *tex, size_t index, bool pause) {
  return;
//        SetTargetFPS(0);
  static bool fuckin_manual_pause_iguess = false;
  static Camera2D cam = {0};
  if(!cam.zoom) cam.zoom = (float)GetRenderWidth()/RES_X;
  static int debug_color = 0;
  const float dbg_cam_step = 100;
  const float dbg_cam_zoom = 1.5;

  (pause || fuckin_manual_pause_iguess) ? SetTargetFPS(60) : SetTargetFPS(0);

  

  for(;;) {
    switch(GetKeyPressed()) {
      case KEY_UP:
        cam.offset.y += dbg_cam_step;
        break;
      case KEY_DOWN:
        cam.offset.y -= dbg_cam_step;
        break;
      case KEY_RIGHT:
        cam.offset.x += dbg_cam_step;
        break;
      case KEY_LEFT:
        cam.offset.x -= dbg_cam_step;
        break;
      case KEY_W:
        cam.zoom *= dbg_cam_zoom;
        break;
      case KEY_S:
        cam.zoom /= dbg_cam_zoom;
        break;
      case KEY_SPACE:
        Vector2 mouse_pos = 
          Vector2Multiply(GetMousePosition(), (Vector2){(double)RES_X / WINDOW_SIZE_X, (double)RES_Y / WINDOW_SIZE_Y});
        //mouse_pos = Vector2Divide(mouse_pos, (Vector2){cam.zoom, cam.zoom});
        printf("%f, %f (%lu)\n", mouse_pos.x, mouse_pos.y, ((size_t)mouse_pos.y * RES_X) + (size_t)mouse_pos.x);
        break;
      case KEY_ENTER:
        return;
      default:
        BeginDrawing();
        pix[index] = 
          (Color) {debug_color, pix[index].g, 0, 255};
        BeginDrawing();
        UpdateTexture(*tex, pix);
        DrawTextureEx(*tex, (Vector2)
            {0 - cam.offset.x, cam.offset.y}, 0, 
            cam.zoom, WHITE);
        EndDrawing();
        if(!pause && !fuckin_manual_pause_iguess) return;
    }
  }
}

//only need four indecies, however we use 8 because it's more convinient. 
void detect_borders(bool borders[8], size_t i) {
  //if this is too slow, it's easy to do it without the modulos.
  int index_mod = i % RES_X;
  bzero(borders, sizeof(*borders) * 8);
  if((i + RES_X) > (RES_X * RES_Y)) {
    for(int nei_dir = SE; nei_dir <= SW; nei_dir++)
      borders[nei_dir] = GCHAN_EXTERNAL;
  }
  else if(((int)i - RES_X) < 0) {
    borders[NE] = GCHAN_EXTERNAL;
    borders[N] = GCHAN_EXTERNAL;
    borders[NW] = GCHAN_EXTERNAL;
  }
  if(index_mod == 0) {
    for(int nei_dir = SW; nei_dir < NW; nei_dir++)
      borders[nei_dir] = GCHAN_EXTERNAL;
  }
  else if(index_mod == (RES_X - 1)) {
    for(int nei_dir = NE; nei_dir < SE; nei_dir++)
      borders[nei_dir] = GCHAN_EXTERNAL;
  }
}

void debug_nei_arrays(int *priority, int *presort, size_t index) {
  int debug_x = index % RES_X;
  int debug_y = index / RES_X;
  printf("(%i, %i) %lu: pre [", debug_x, debug_y, index);
  for(int nd = 0; nd < 8; nd++) printf("%i, ", presort[nd]);
  printf("], pri [");
  for(int nd = 0; nd < 8; nd++) printf("%i, ", priority[nd]);
  printf("]\n");
}

enum {
  SCAN_MODE_NONE,
  SCAN_MODE_SAFE,
  SCAN_MODE_INTERIOR
} scan_mode;

unsigned int mandelbrot_bordertrace(struct camera *cam, Color *pixels) {
  //these lookup tables r cheap cuz on the stm32f1, 1 memory read is 1 instruction
  FixedCord scale = { 
    .r = DOUBLE_TO_FIXED((cam->max_r - cam->min_r) / (double)RES_X), 
    .i = DOUBLE_TO_FIXED((cam->max_i - cam->min_i) / (double)RES_Y)};
  FixedCord c = {.r = 0, .i = DOUBLE_TO_FIXED(cam->max_i)};
  unsigned int total_iters = 0;
  size_t on_pixel = 0;
  int border_scanning = 0;

  

  Image img = GenImageColor(RES_X, RES_Y, BLUE);
  Texture debug_tex = LoadTextureFromImage(img);
  UnloadImage(img);
//  bzero(pixels, RES_X * RES_Y * sizeof(Color));
//  for(size_t c = 0; c < (RES_X * RES_Y); c++) pixels[c] = (Color){0,0,0,255};
  

  for(int y = 0; y < RES_Y; y++) {
    border_scanning = 0;
    c.r = DOUBLE_TO_FIXED(cam->min_r);
    for(int x = 0; x < RES_X; x++) {

      //c.r = DOUBLE_TO_FIXED((((on_pixel % RES_X) / (double)RES_X) * (cam->max_r - cam->min_r)) + cam->min_r);
      //c.i = DOUBLE_TO_FIXED((((on_pixel / (double)RES_X) / (double)RES_Y) * (cam->min_i - cam->max_i)) + cam->max_i);

      switch(pixels[on_pixel].g) {
        case GCHAN_UNRENDERED:
          if(border_scanning) {
            //pixels[on_pixel] = get_color(ITERS);
            //printf("interior\n");
            pixels[on_pixel] = (Color){0xfe,0,0xfe,0xff};
            break;
          }
          //printf("rendering %i, %i (%lu)\n", x, y, on_pixel);
          int i = iterate(c);
          total_iters += i;
          pixels[on_pixel] = get_color(i);
          if(i == ITERS) {
            FixedCord this_coord = c;
            size_t this_index = on_pixel;
            bool seperated_from_start = false;
            bool nei_canidate[8];
            int last_nei_canidate[8];
            int nei_presort[8];

            size_t backstack[BACKSTACK_SIZE];
            size_t backstack_i = 0;
            int backstack_calls = 0;

            int nei_dir;

            debug_step(pixels, &debug_tex, this_index, false);
            bool debug_mode = false;


            
            bool borders[8];
            detect_borders(borders, on_pixel);
            for(nei_dir = 0; nei_dir < 8; nei_dir++) {
              size_t nei_i;
              if(borders[nei_dir]) break;
              nei_i = get_neighbor_index(on_pixel, nei_dir);
              if(pixels[nei_i].g & GCHAN_EXTERNAL) break;
            }

            if(nei_dir >= 8) {
              border_scanning = SCAN_MODE_INTERIOR;
              break;
            }

            while(true) {
              detect_borders(borders, this_index);
              debug_step(pixels, &debug_tex, this_index, false);
              if(debug_mode) debug_step(pixels, &debug_tex, on_pixel, debug_mode);
              
              //step 1: check pixels around us, fill in neighbors.
              bzero(nei_presort, sizeof(nei_presort));

              this_coord.r = DOUBLE_TO_FIXED((((this_index % RES_X) / (double)RES_X) * (cam->max_r - cam->min_r)) + cam->min_r);
              this_coord.i = DOUBLE_TO_FIXED((((this_index / (double)RES_X) / (double)RES_Y) * (cam->min_i - cam->max_i)) + cam->max_i);
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

                nei_i = get_neighbor_index(this_index, nei_dir);
                gchan_info = pixels[nei_i].g;
                if(nei_i == on_pixel) start_is_nei = true;
                //note that when we move this over, there will be no alpha channel.
                //gchan_info will be extracted differently!!!
                if(gchan_info) nei_presort[nei_dir] = gchan_info;
                else {
                  int i = iterate(get_neighbor_coord(this_coord, nei_dir, scale));
                  pixels[nei_i] = get_color(i);
                  nei_presort[nei_dir] = (i >= ITERS) ? GCHAN_INTERNAL : GCHAN_EXTERNAL;
                  pixels[nei_i].g = nei_presort[nei_dir];
                }
              }
              if(!start_is_nei && !seperated_from_start && (this_index != on_pixel)) seperated_from_start = true;
              if(start_is_nei && seperated_from_start) {
                pixels[this_index].g = GCHAN_BLOCKED;
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
              pixels[this_index].g = GCHAN_BLOCKED;
              for(int nei_dir = 0; nei_dir < 8; nei_dir += 2) {
                if(!nei_canidate[nei_dir]) continue;
                backstack_calls = 0;
                this_index = get_neighbor_index(this_index, nei_dir);
                this_coord = get_neighbor_coord(this_coord, nei_dir, scale);
                goto NEXT_PIXEL;
              }
              if((backstack_calls++ > BACKSTACK_SIZE) || (backstack_i < 1)) break;
              this_index = backstack[--backstack_i % BACKSTACK_SIZE];
              NEXT_PIXEL:
              for(int i = 0; i < 8; i++) nei_canidate[i] = false;
            }
            debug_step(pixels, &debug_tex, this_index, true);
          }
          else pixels[on_pixel].g = GCHAN_EXTERNAL;
          break;
          /**
        case GCHAN_INNER_CLOSED:
          if(((x + 2) < RES_X) && (pixels[on_pixel + 1].g == GCHAN_UNRENDERED)) border_scanning = SCAN_MODE_NONE;
          break;
          **/
        default: 
          border_scanning = SCAN_MODE_NONE;
      }
      on_pixel++;
      c.r += scale.r;
    }
    c.i += scale.i;
    border_scanning = false;
  }
  debug_step(pixels, &debug_tex, 0, true);
  for(size_t i = 0; i < (RES_X * RES_Y); i++) pixels[i].g = 0;
  return total_iters;
}

unsigned int mandelbrot_unoptimized(struct camera *cam, Color *pixels) {
  FixedCord scale = { 
    .r = DOUBLE_TO_FIXED((cam->max_r - cam->min_r) / (double)RES_X), 
    .i = DOUBLE_TO_FIXED((cam->max_i - cam->min_i) / (double)RES_Y)};
  FixedCord c = { .r = DOUBLE_TO_FIXED(cam->min_r), .i = DOUBLE_TO_FIXED(cam->max_i) };
  unsigned int total_iters = 0;
  size_t on_pixel = 0;
  for(int y = 0; y < RES_Y; y++) {
    c.r = DOUBLE_TO_FIXED(cam->min_r);
    for(int x = 0; x < RES_X; x++) {
      int i = iterate(c);
      //c.r = DOUBLE_TO_FIXED((((on_pixel % RES_X) / (double)RES_X) * (cam->max_r - cam->min_r)) + cam->min_r);
      //c.i = DOUBLE_TO_FIXED((((on_pixel / (double)RES_X) / (double)RES_Y) * (cam->min_i - cam->max_i)) + cam->max_i);
      total_iters += i;
      pixels[((y * RES_X) + x)] = get_color(i);
      on_pixel++;
      c.r += scale.r;      
    }
    c.i -= scale.i;
  }
  return total_iters;
}

int main() {
  //test();
  //return 0;
  Color *pixels_unoptimized = malloc(RES_X * RES_Y * sizeof(Color));
  Color *pixels_optimized = malloc(RES_X * RES_Y * sizeof(Color));
  bool optimized = false;
  //(1.514379082621093886019, 0.000033222739567139065) - (1.514381385800912305228, 0.000034374329476534746)

  struct camera cam_default = {
    .min_r = -1,
    .max_r = 1
  };
  cam_default.min_i = ((double)RES_Y / RES_X) * cam_default.min_r;
  cam_default.max_i = ((double)RES_Y / RES_X) * cam_default.max_r;
    

  //done
  //.min_r = 0.340060821337554164412, .min_i = -0.076399869494282027227, .max_r = 0.340671385211165078655, .max_i = -0.076094587557451340287 

  //done
  //.min_r = 0.348347456407892719366, .min_i = -0.092130353675640097588, .max_r = 0.349033773135021985201, .max_i = -0.091787195312047098472


  //has internal noise 
  //.min_r = 0.348416088080605645949, .min_i = -0.092130353675640097588, .max_r = 0.349102404807734911785, .max_i = -0.091787195312047098472

  //needs diagnol transfer
  //.min_r = 0.352126044212195454808, .min_i = -0.101818891004586714599, .max_r = 0.354169737175103083171, .max_i = -0.100797044523048578979

  //works
  //.min_r = 1.514379082621093886019, .min_i = 0.000033222739567139065, .max_r = 1.514381385800912305228, .max_i = 0.000034374329476534746

//  unusual issue; complete rendered border
  //  .min_r = 0.426539347230382670517, .min_i = 0.218210183100018217939, .max_r = 0.427445609943903681582, .max_i = 0.218663314456816582076 


  
  struct camera cam = {
    //.min_r = 0.348416088080605645949, .min_i = -0.092130353675640097588, .max_r = 0.349102404807734911785, .max_i = -0.091787195312047098472
    .min_r = 0.348347456407892719366, .min_i = -0.092130353675640097588, .max_r = 0.349033773135021985201, .max_i = -0.091787195312047098472
  };
  InitWindow(WINDOW_SIZE_X, WINDOW_SIZE_Y, "mandelbrot fixed point test");
  SetTraceLogLevel(LOG_ERROR);

  Image img = GenImageColor(RES_X, RES_Y, BLUE);
  Texture tex = LoadTextureFromImage(img);
  UnloadImage(img);

  SetTargetFPS(60);

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
      case KEY_SPACE:
        optimized = !optimized;
        break;
      default:
        BeginDrawing();
        EndDrawing();
        continue;
        break;
    }
    printf(".min_r = %.21f, .min_i = %.21f, .max_r = %.21f, .max_i = %.21f\n", cam.min_r, cam.min_i, cam.max_r, cam.max_i);


    clock_t begin, end;
    double time_unoptimized;
    double time_optimized;

    for(int i = 0; i < (RES_X * RES_Y); i++) { pixels_unoptimized[i] = (Color){0, 0, 0, 0xff}; }
    for(int i = 0; i < (RES_X * RES_Y); i++) { pixels_optimized[i] = (Color){0, 0, 0, 0xff}; }

    begin = clock();
    unsigned int unoptimized_iters = mandelbrot_unoptimized(&cam, pixels_unoptimized);
    end = clock();

    time_unoptimized = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Unoptimized: %u iterations, %f seconds\n", unoptimized_iters, time_unoptimized);

    begin = clock();
    unsigned int optimized_iters = mandelbrot_bordertrace(&cam, pixels_optimized);
    end = clock();

    time_optimized = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Border tracing: %u iterations, %f seconds\n", optimized_iters, time_optimized); 

    printf("border tracing does %f%% of nieve approach\n", ((float)optimized_iters / unoptimized_iters) * 100);

    BeginDrawing();
    printf("%s\n", optimized ? "optimized mode" : "unoptimized mode");
    UpdateTexture(tex, optimized ? pixels_optimized : pixels_unoptimized);
    DrawTextureEx(tex, (Vector2){0,0}, 0, (float)GetRenderWidth()/RES_X, WHITE);
    EndDrawing();

  }

  return 0;
}
