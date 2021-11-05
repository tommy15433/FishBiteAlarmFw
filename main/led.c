#include <stdio.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_attr.h"
#include "soc/rtc.h"
#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

#include "esp_log.h"

#include "led.h"


#define GPIO_LED_RED    18
#define GPIO_LED_GRN  19

#define LED_TAG "LED_PORT"
#define LED_PWM_TAG "LED_PWM"

static mcpwm_config_t pwm_config;

typedef struct{
    double brightness;
    int enableValue;
}led_ctrl;

static led_ctrl led_ctrl_red = {
    .brightness = 50.0,
    .enableValue = 1
};
static led_ctrl led_ctrl_grn = {
    .brightness = 50.0,
    .enableValue = 1
};

void led_set_ctrl(void)
{
    pwm_config.cmpr_a = led_ctrl_red.brightness * led_ctrl_red.enableValue;
    pwm_config.cmpr_b = led_ctrl_grn.brightness * led_ctrl_grn.enableValue;
    
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
    ESP_LOGI(LED_PWM_TAG, "RED: %f, GRN: %f", pwm_config.cmpr_a, pwm_config.cmpr_b);
}

void led_set_colorRed(void) 
{ 
    led_ctrl_red.enableValue = 1;
    led_ctrl_grn.enableValue = 0;

    led_set_ctrl();
}

void led_set_colorGrn(void) 
{ 
    led_ctrl_red.enableValue = 0;
    led_ctrl_grn.enableValue = 1;

    led_set_ctrl();
}
void led_set_colorYel(void)
{
    led_ctrl_red.enableValue = 1;
    led_ctrl_grn.enableValue = 1;

    led_set_ctrl();
}

void led_set_brightness(double val)
{
    if (val > LED_PWM_MAX) val = LED_PWM_MAX;
    if (val < LED_PWM_MIN) val = LED_PWM_MIN;

    led_ctrl_red.brightness = val;
    led_ctrl_grn.brightness = val;

    led_set_ctrl();
}

void led_init(void)
{
    ESP_LOGI(LED_TAG, "RED: %d, GRN: %d", GPIO_LED_RED, GPIO_LED_GRN);

    mcpwm_pin_config_t pin_config = {
        .mcpwm0a_out_num = GPIO_LED_RED,
        .mcpwm0b_out_num = GPIO_LED_GRN,
    };
    mcpwm_set_pin(MCPWM_UNIT_0, &pin_config);

    pwm_config.frequency = 1000;    //frequency = 1000Hz
    pwm_config.cmpr_a = 50.0;       //duty cycle of PWMxA = 60.0%
    pwm_config.cmpr_b = 50.0;       //duty cycle of PWMxb = 50.0%
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);

    led_set_colorGrn();
    led_set_brightness(50);
}

#define LED_IO      2
#define LED_PIN_SEL (1ULL << LED_IO)

static int led_status = 0;

void led_debug_init(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = LED_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}

void led_debug_toggle(void)
{
    gpio_set_level(LED_IO, ++led_status % 2);
}