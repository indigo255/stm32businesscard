#include <signal.h>
#include "main.h"

static float time __attribute__((used));

void benchmark_start() {
  TIM4->CNT = 0;
  TIM4->CR1 |= TIM_CR1_CEN;
}
void benchmark_stop() {
  TIM4->CR1 ^= TIM_CR1_CEN;
  time = (float)(TIM4->CNT + 1) * (TIM4->PSC + 1) / HAL_RCC_GetSysClockFreq();
  BENCHMARK_CHECKTIME:
  __NOP();
//  __BKPT();
}
