#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <raylib.h>
#include <raymath.h>
#include <limits.h>

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
  if((i == ITERS) || (i == 0)) return (Color){0, 0, 0, 255};
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

int main() {
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
    printf("(%.6f, %.6f) - (%.6f, %.6f)\n", cam.min_r, cam.min_i, cam.max_r, cam.max_i);
    {
      int32_t scale_i = DOUBLE_TO_FIXED((cam.max_i - cam.min_i) / (double)RES_Y);
      int32_t scale_r = DOUBLE_TO_FIXED((cam.max_r - cam.min_r) / (double)RES_X);
      int32_t c_i = DOUBLE_TO_FIXED(cam.max_i);
      int32_t c_r, z_i, z_r, zn_i, zn_r;
      int32_t z_r_2, z_i_2;

      int i;
      int min = INT_MAX;
      int max = INT_MIN;
      for(int y = 0; y < RES_Y; y++) {
        c_r = DOUBLE_TO_FIXED(cam.min_r);
        for(int x = 0; x < RES_X; x++) {
          z_i = 0;
          z_r = 0;
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
          if(i > max) max = i;
          if(i < min) min = i;
          pixels[((y * RES_X) + x)] = get_color(i);
          c_r += scale_r;
        }
        c_i -= scale_i;
      }
    }

    BeginDrawing();
    UpdateTexture(tex, pixels);
    DrawTextureEx(tex, (Vector2){0,0}, 0, (float)GetRenderWidth()/RES_X, WHITE);
    EndDrawing();
  }

  return 0;
}
