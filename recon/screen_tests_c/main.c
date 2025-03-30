#include <stdio.h>
#include <raylib.h>
#include <raymath.h>

#define RES_X 160
#define RES_Y 80
#define DEFAULT_CENTER_X 0
#define DEFAULT_CENTER_Y 0
#define MOUSE_BUTTON 0
#define STEP_SIZE .25
#define ZOOM_SIZE .25

#define INFTY 4
#define INFTY_SQR INFTY * INFTY
#define ITERS 8

struct camera {
  double min_r, min_i, max_r, max_i;
};

//god damb maybe I shoulda made a vector class
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
  struct camera cam = {
    .min_r = -1,
    .max_r = 1,
    .min_i = -1,
    .max_i = 1
  };
  InitWindow(160, 80, "mandelbrot fixed point test");

  Image img = GenImageColor(RES_X, RES_Y, BLUE);
  Texture tex = LoadTextureFromImage(img);

  SetTargetFPS(30);
  while(!WindowShouldClose()) {
    if(key_pressed = GetKeyPressed()) {
      switch(key_pressed) {
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
          break;
      }
      printf("(%f, %f) - (%f, %f)\n", cam.min_r, cam.min_i, cam.max_r, cam.max_i);
    }
    {
      double scale_i = (cam.max_i - cam.min_i) / (double)GetScreenHeight();
      double scale_r = (cam.max_r - cam.min_r) / (double)GetScreenWidth();
      double c_i = cam.min_i;
      double c_r = cam.min_r;
      double z_i = 0;
      double z_r = 0;
      for(int y = 0; y < RES_Y; y++) {
        for(int x = 0; x < RES_Y; x++) {
          for(int i = 0; i < ITERS; i++) {

            if(pow(c_i, 2) + pow(c_r, 2) >= INFTY_SQR) {
              break;
            }
          }
          c_r += scale_r;
        }
        c_i += scale_i;
      }
    }

    BeginDrawing();
    DrawTexture(tex, 0, 0, WHITE);
    EndDrawing();
  }

  return 0;
}
