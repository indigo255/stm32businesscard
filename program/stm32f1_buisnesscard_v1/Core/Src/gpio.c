#include "gpio.h"
#include <stdint.h>
#include <stdbool.h>


int gpio_event = EVENT_NONE;

void HAL_GPIO_EXTI_Callback(uint16_t pin) { 
  //implimented demo functionality last second; maybe consider moving to a different interrupt
  uint32_t pin_mult = (((BUTTON_REG_1->IDR) & BUTTON_R1_MASK) | ((BUTTON_REG_2->IDR) & BUTTON_R2_MASK));
  uint32_t fuck1 = BUTTON_REG_1 -> IDR;
  uint32_t fuck2 = BUTTON_REG_2 -> IDR;
  if((pin_mult & (pin_mult - 1))) {
    gpio_event = pin_mult;
  }
  else gpio_event = pin; 
}

int event_get() {
  int event = gpio_event;
  gpio_event = EVENT_NONE;
  return event;
}

int event_peek() { return gpio_event; }

bool charging_batt() { return HAL_GPIO_ReadPin(CHARGING_PORT, CHARGING_PIN); }
