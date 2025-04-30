# STM32 Mandelbrot Explorer Buisness Card
A battery powered business card that can explore the Mandelbrot set. Meant to be cheaply made for handing out. Will likely include a flappy bird clone to encourage recruiter competition.
# Project in development! See below for a brief write up.
![STM32 Card Prototype 1](https://git.bpcspace.com/indigo/stm32_business_card/raw/branch/main/preview.png)

## Project structure
Code: [program/stm32f1_buisnesscard_v1/Core](https://git.bpcspace.com/indigo/stm32_business_card/src/branch/main/program/stm32f1_buisnesscard_v1/Core) <br>
PCB: [kicad/](https://git.bpcspace.com/indigo/stm32_business_card/src/branch/main/kicad)


## Software Development
The software is currently working, but is more a proof of concept (messy and unoptimized). In it's current state, I'm leaning heavily on ST's HAL to save time. For a project where everything is written from the ground up, see my other projects below [^1]!

### Display
I've modified [this repository](https://github.com/afiskon/stm32-st7735) to fit my needs. I've had to make some minor changes to utilize the ST7735's sleep mode, and to make the code work for my specific display. I expect I'll need to rewrite the library to manipulate raw registers to optimize SPI communication, as it currently uses HAL and is *very* slow. I'm considering implementing a game as well, which may require I access SPI via DMA.
### (No) FPU
The STM32F1 lineup doesn't have an FPU, thus fixed point integer arithmetic is used to speed up rendering. The decimal is intentionally too close to the MSB, as this introduces aesthetic visual artifacts at no cost that I think are a nice twist.
*notice the banding around the set- that's an artifact of pushing fixed point arithmatic beyond its limits*
![](https://git.bpcspace.com/indigo/stm32_business_card/src/branch/main/writeup/quick_buildings.png)
![](https://git.bpcspace.com/indigo/stm32_business_card/src/branch/main/writeup/quick_spiral.png)

### MCU Power Consumption
The software is interrupt based, saving power any time there's not an active job. After 30 seconds, a MOSFET will turn off the backlight and the MCU will in a deeper sleep state, where only a few microamps are consumed. This allows the card to operate without a power switch, as theoretically the sleeping power consumption is insignificant compared to the shelf life of these batteries (a few months).

## Hardware Development
The PCB works, and is what I'm using to test code. I'm planning on making some changes, including adding MOSFETS for the display backlight and creating some art for the silk screen. 
The hardware is intentionally left simple for rapid development; there's two ideal diodes, allowing power delivery via USB-C or batteries, a step-down switching regulator, buttons with debouncing circuits, and of course, the MCU.
Some of the most vital components are described below.

### Batteries
This card will be powered by 4 zinc air batteries (cost effective & high power density, commonly used in hearing aids). The downside to these batteries is that they're non-rechargeable, and after activation, they only have a shelf life of a few weeks. Batteries are expensive, and what I've got is more then enough- it may actually be overkill.
A battery holder was developed to be used with springs to hold the batteries in place. I haven't ordered the batteries yet, so I have yet to see how it performs.

### Display
The display is the cheapest 2$ 65k 160x80 color LCD I could find on LCSC. It is the most expensive part of my card, matched with batteries. I'm sure I could have found it on Ali Express for 50¢, but hindsight is 20/20. It's actually very pretty, and I plan to upload images soon.

### SPI troubles
Currently, SPI only works consistently at 4mHZ. I have yet to probe it with my oscilloscope and figure out what's wrong; I'm guessing parasitic capacitance. My next PCB will have differential routing to allow higher speeds.

## Things I'd change
I'm used to working with a lot lower spec MCUs, and after the purchase I've found that the price of the STM32f1 is actually quite expensive for it's performance. I might want to try something uber cheap for my next project requiring a higher speed 32 bit processor, as long as it's got a HAL to accelerate development. The ch32v003 looks pretty cool...

## Other Projects
[^1]: For another embedded project without any assistance from HAL, check out my (unfinished) [AVR wristwatch](https://git.bpcspace.com/indigo/AVRwristwatch), where everything- from the I2C display and RTC clock is developed from the ground up! 
For a much larger project that's not exactly embedded, check out my [Operating System](https://git.bpcspace.com/indigo/IndigoOS). I haven't been able to work on it since going to Missouri S&T, but some impressive feats include a bootloader, an efficient binary tree/buddy system physical memory allocator, and multi-core execution. It's a lot of code, maybe check it out!
