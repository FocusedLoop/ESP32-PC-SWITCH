#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

#define LED_1 GPIO_NUM_14
#define LED_2 GPIO_NUM_12
#define OUTPUT_PIN_1 GPIO_NUM_26
#define OUTPUT_PIN_2 GPIO_NUM_27

#define BUZZER GPIO_NUM_2

void led_setup()
{
    // GPIO Configuration for LEDs
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << LED_1) | (1ULL << LED_2) | (1ULL << OUTPUT_PIN_1) | (1ULL << OUTPUT_PIN_2); // LED and PC power pins
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // Buzzer Configuration
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_10_BIT, // 10-bit resolution
        .freq_hz = 1000,                      // Default frequency: 1 kHz
        .speed_mode = LEDC_LOW_SPEED_MODE,    // Low-speed mode
        .timer_num = LEDC_TIMER_0             // Use Timer 0
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .channel    = LEDC_CHANNEL_0,         // Use channel 0
        .duty       = 0,                      // duty cycle
        .gpio_num   = BUZZER,                 // GPIO pin for the buzzer
        .speed_mode = LEDC_LOW_SPEED_MODE,    // Low-speed mode
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_0,           // Use Timer 0
    };
    ledc_channel_config(&ledc_channel);

    printf("Setup complete.\n");
}

void pc_pw(){
    gpio_set_level(OUTPUT_PIN_1, 1);
    gpio_set_level(OUTPUT_PIN_2, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_level(OUTPUT_PIN_1, 0);
    gpio_set_level(OUTPUT_PIN_2, 0);
}

void beep()
{
    const int frequencies[] = {261, 440, 880};
    for (int i = 0; i < 3; i++)
    {
        ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, frequencies[i]);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        vTaskDelay(pdMS_TO_TICKS(200));
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void led_loop(int8_t state)
{
    // RED LED
    if (state == 0)
    {
        gpio_set_level(LED_1, 1);
        gpio_set_level(LED_2, 0);
        printf("OFF\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // GREEN LED
    else if (state == 1)
    {
        gpio_set_level(LED_1, 0);
        gpio_set_level(LED_2, 1);
        printf("ON\n");
        beep();
        pc_pw();
    }
}