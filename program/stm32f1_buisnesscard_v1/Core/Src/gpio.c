#include "gpio.h"
#include <stdint.h>
#include <stdbool.h>

int gpio_event = EVENT_NONE;
void HAL_GPIO_EXTI_Callback(uint16_t pin) { gpio_event = pin; }
int event_get() {
  int event = gpio_event;
  gpio_event = EVENT_NONE;
  return event;
}

int event_peek() { return gpio_event; }

bool charging_batt() { return !HAL_GPIO_ReadPin(GPIO_EXTI_REG, CHARGING_PIN); }
//bool charging_batt() { return ~HAL_GPIO_ReadPin(GPIO_EXTI_REG, CHARGING_PIN); }
