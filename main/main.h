#pragma once

#include "driver/gpio.h"
#include "driver/rtc_io.h"

#define GPIO_RESET_NO       27
#define GPIO_RESET_PINSEL   (1ULL << GPIO_RESET_NO)
#define GPIO_RESET_LEVEL    GPIO_INTR_POSEDGE
#define GPIO_RESET_PRESSED  1
#define GPIO_RESET_RELEASED 0

