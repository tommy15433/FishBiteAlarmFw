#pragma once

enum{
    LED_RED = 0,
    LED_GRN = 1
}LEDs;

static const double LED_PWM_MAX = 100.0;
static const double LED_PWM_MIN = 0.0;

void led_set_colorRed(void);
void led_set_colorGrn(void);
void led_set_colorYel(void);
void led_set_brightness(double val);

void led_init(void);

void led_debug_init(void);
void led_debug_toggle(void);