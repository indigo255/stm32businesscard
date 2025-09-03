#include "stm32f1xx_hal.h"
#include "st7735.h"
#include "gpio.h"
#include "idle.h"
#include "main.h"


void idle() {
  while(charging_batt() && (event_peek() == EVENT_NONE)) {
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
  }
  if(!charging_batt() && (event_peek() == EVENT_NONE)) {
    HAL_TIM_Base_Stop_IT(&htim2);
    HAL_SuspendTick();
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    HAL_TIM_Base_Start_IT(&htim2);
    //in this power state we still recieve timer interrupts
    HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI); //TODO ajust time
  }

  SystemClock_Config();
  HAL_ResumeTick();

  if(!charging_batt() && (event_peek() == EVENT_NONE)) {
    ST7735_sleep();
    HAL_SuspendTick();
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    SystemClock_Config();
    HAL_ResumeTick();
    ST7735_wake();
    event_get();
  }
}
