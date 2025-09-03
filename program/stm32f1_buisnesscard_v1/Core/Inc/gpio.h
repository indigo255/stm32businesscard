#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx_hal.h"

/** all external interupts will be on GPIOB due to hardware layout and software convinience
  TODO move to gpio.h or somethin
**/

#define GPIO_EXTI_REG       GPIOB
#define BUTTON_UP           GPIO_PIN_3
#define BUTTON_RIGHT        GPIO_PIN_2
#define BUTTON_DOWN         GPIO_PIN_0
#define BUTTON_LEFT         GPIO_PIN_1
#define BUTTON_A            GPIO_PIN_13
#define BUTTON_B            GPIO_PIN_14
#define CHARGING_PIN        GPIO_PIN_12
#define EVENT_NONE          0 

extern int button;
void HAL_GPIO_EXTI_Callback(uint16_t pin);
int event_get();
int event_peek();
bool charging_batt();
