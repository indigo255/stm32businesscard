#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>

#define RES_X 160
#define RES_Y 80
#define DEFAULT_CENTER_X 0
#define DEFAULT_CENTER_Y 0
#define MOUSE_BUTTON 0
#define STEP_SIZE .25
#define ZOOM_SIZE .25

#define INFTY 2
#define INFTY_SQR INFTY * INFTY
#define ITERS 255

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
  int key_pressed;
  Color *pixels = malloc(RES_X * RES_Y * sizeof(Color));
  struct camera cam = {
    .min_r = -1,
    .max_r = 1,

            yru += di*(targety - yru)/10.;
        }    .min_i = -1,
    .max_i = 1
  };
  InitWindow(RES_X, RES_Y, "mandelbrot fixed point test");

  Image img = GenImageColor(RES_X, RES_Y, BLUE);
  Texture tex = LoadTextureFromImage(img);
  UnloadImage(img);

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
        printf("a\n");
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
    printf("(%f, %f) - (%f, %f)\n", cam.min_r, cam.min_i, cam.max_r, cam.max_i);
    {
      //double scale_i = (cam.max_i - cam.min_i) / (double)GetRenderHeight();
      //double scale_r = (cam.max_r - cam.min_r) / (double)GetRenderWidth();
      double scale_i = (cam.max_i - cam.min_i) / (double)RES_Y;
      double scale_r = (cam.max_r - cam.min_r) / (double)RES_X;
      double c_i = cam.max_i;
      double c_r;
      double z_i;
      double z_r;
      double zn_r, zn_i;
      int i;
      uint8_t color;
      for(int y = 0; y < RES_Y; y++) {
        c_r = cam.min_r;
        for(int x = 0; x < RES_X; x++) {
          z_i = 0;
          z_r = 0;
          for(i = 0; i < ITERS; i++) {
            zn_r = pow(z_r, 2.0) - pow(z_i, 2.0) + c_r;
            zn_i = (2.0 * z_r * z_i) + c_i;
            z_i = zn_i;
            z_r = zn_r;
            if((pow(z_i, 2.0) + pow(z_r, 2.0)) > INFTY_SQR) {
              break;
            }
          }
          color = ((float)i / ITERS) * UINT8_MAX;
          pixels[((y * RES_X) + x)] = (Color){color, color, color, 255};
          c_r += scale_r;
        }
        c_i -= scale_i;
      }
    }

    BeginDrawing();
    UpdateTexture(tex, pixels);
    DrawTexture(tex, 0, 0, WHITE);
    EndDrawing();
  }

  return 0;
}
