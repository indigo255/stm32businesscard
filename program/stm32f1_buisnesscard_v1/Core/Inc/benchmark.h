#pragma once
#include <signal.h>
#include "main.h"

static float time __attribute__((used));

__attribute__((always_inline)) void static inline benchmark_start() {
  TIM4->CNT = 0;
  TIM4->CR1 |= TIM_CR1_CEN;
}
__attribute__((always_inline)) void static inline benchmark_stop() {
  TIM4->CR1 ^= TIM_CR1_CEN;
  time = (float)(TIM4->CNT + 1) * (TIM4->PSC + 1) / HAL_RCC_GetSysClockFreq();
  __BKPT();
}
