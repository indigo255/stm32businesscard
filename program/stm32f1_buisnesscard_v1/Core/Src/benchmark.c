#include <signal.h>
#include "mandelbrot.h"
#include "main.h"
#include "st7735.h"
#include "gpio.h"

//static float time __attribute__((used));

//whatever I'm tired of using hal
void benchmark_start() {
  TIM4->CNT = 0;
  TIM4->CR1 |= TIM_CR1_CEN;
}

float benchmark_stop() {
  TIM4->CR1 ^= TIM_CR1_CEN;
  return (float)(TIM4->CNT + 1) * (TIM4->PSC + 1) / HAL_RCC_GetSysClockFreq();
}

void benchmark() {
  uint16_t scheme[256]; //yeah eyah yeah constant scope balalse';hjf v'wef
  init_colorscheme(scheme);

  const struct camera cases[] = {
    //shows child mandelbrot at about 1/4th of the screen 
    (struct camera) {1.511138965827779623297, -0.000099833545397436595, 1.513299500557251375810, 0.000980433819429512915},

    //main set taking up most of screen
    (struct camera) {-1.006631762350983283483, 0.033264514201812478422, 0.159212948008334742589, 0.616186869452269969649},

    //main set taking up a little bit of the screen
    (struct camera) {-3.860919485929509509248, -1.895171233654595388529, 4.122278116347564136390, 2.096427567968737193382},

    //zoomed in far, set taking up half of screen
    (struct camera) {0.250175465354113391037, -0.000006169573538442441, 0.250185172171380920680, -0.000001316164904004028},

    //zoomed in far, set taking up none of screen
    (struct camera) {0.250177970233267155109, -0.000003445496138560340, 0.250181935207702044188, -0.000001463008920807711},

    //half and half, complex border
    (struct camera) {3.530034617447952438596, -0.011581384229626515842, 3.530490903206738639852, -0.011353241350196253967},

    //complete blackness
    (struct camera) {-1.097044987707582519576, 0.142802338676453366428, -1.000760853479837342306, 0.190944405713568132743}
  };

  const int case_cnt = sizeof(cases) / sizeof(*cases);

  static float unoptimized_times[7] __attribute__((used));
  static float optimized_times[7] __attribute__((used));

  uint16_t framebuffer[(RES_X/2) * RES_Y] = {0};
  const struct window left_half = {0, 0, 80, 80};
  const struct window right_half = {80, 0, 80, 80};
  

  for(int case_i = 0; case_i < case_cnt;  case_i++) {

    benchmark_start();
    mandelbrot_bordertrace(framebuffer, scheme, cases[case_i], left_half);
    ST7735_DrawImage(0, 0, 80, 80, framebuffer);
    mandelbrot_bordertrace(framebuffer, scheme, cases[case_i], right_half);
    ST7735_DrawImage(80, 0, 80, 80, framebuffer);
    optimized_times[case_i] = benchmark_stop();
 //   while(!event_get());
    
    benchmark_start();
    mandelbrot_unoptimized(framebuffer, scheme, cases[case_i], left_half);
    ST7735_DrawImage(0, 0, 80, 80, framebuffer);
    mandelbrot_unoptimized(framebuffer, scheme, cases[case_i], right_half);
    ST7735_DrawImage(80, 0, 80, 80, framebuffer);
    unoptimized_times[case_i] = benchmark_stop();
//    while(!event_get());

  }
  __BKPT();
}
