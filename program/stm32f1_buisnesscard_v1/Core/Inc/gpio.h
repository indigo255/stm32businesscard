#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx_hal.h"

/** all external interupts will be on GPIOB due to hardware layout and software convinience
  TODO move to gpio.h or somethin
**/

#define EVENT_NONE          0 

#define BUTTON_REG_1        GPIOA
#define BUTTON_REG_2        GPIOB
#define BUTTON_R1_MASK      (BUTTON_UP | BUTTON_DOWN | BUTTON_RIGHT | BUTTON_LEFT)
#define BUTTON_R2_MASK      (BUTTON_A | BUTTON_B)

#define BUTTON_UP           GPIO_PIN_3
#define BUTTON_RIGHT        GPIO_PIN_2
#define BUTTON_DOWN         GPIO_PIN_0
#define BUTTON_LEFT         GPIO_PIN_1
#define BUTTON_A            GPIO_PIN_13
#define BUTTON_B            GPIO_PIN_14

#define CHARGING_PORT       GPIOB
#define CHARGING_PIN        GPIO_PIN_12

#define DEMO_NEXTVIEW       BUTTON_A | BUTTON_B | BUTTON_RIGHT
#define DEMO_LASTVIEW       BUTTON_A | BUTTON_B | BUTTON_LEFT
#define DEMO_BORDER         BUTTON_A | BUTTON_B | BUTTON_UP

extern int button;
void HAL_GPIO_EXTI_Callback(uint16_t pin);
int event_get();
int event_peek();
bool charging_batt();
