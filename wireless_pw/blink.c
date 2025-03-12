#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

#define LED GPIO_NUM_5
#define OUTPUT_PIN GPIO_NUM_19
#define INPUT_PIN GPIO_NUM_18

#define BUZZER GPIO_NUM_4

void led_setup()
{
    // LED Configuration
    gpio_config_t led_conf;
    led_conf.intr_type = GPIO_INTR_DISABLE;
    led_conf.mode = GPIO_MODE_OUTPUT;
    led_conf.pin_bit_mask = (1ULL << LED);
    led_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    led_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&led_conf);

    // OUTPUT_PIN Configuration
    gpio_config_t output_conf;
    output_conf.intr_type = GPIO_INTR_DISABLE;
    output_conf.mode = GPIO_MODE_OUTPUT;
    output_conf.pin_bit_mask = (1ULL << OUTPUT_PIN);
    output_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    output_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&output_conf);

    // INPUT_PIN Configuration
    gpio_config_t input_conf;
    input_conf.intr_type = GPIO_INTR_DISABLE;
    input_conf.mode = GPIO_MODE_INPUT;
    input_conf.pin_bit_mask = (1ULL << INPUT_PIN);
    input_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    input_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&input_conf);

    // Buzzer Configuration
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 1000,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .channel    = LEDC_CHANNEL_0,
        .duty       = 0,
        .gpio_num   = BUZZER,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_0,
    };
    ledc_channel_config(&ledc_channel);

    printf("Setup complete.\n");
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

int led_loop(int8_t state) {
    switch (state) {
        case 1: // Turn On State
            printf("ON\n");
            beep();
            gpio_set_level(OUTPUT_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(1000));
            gpio_set_level(OUTPUT_PIN, 0);
            return 1;

        case 2: // Turn Off State
            printf("OFF\n");
            gpio_set_level(OUTPUT_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(5000));
            gpio_set_level(OUTPUT_PIN, 0);
            return 2;

        default: // Read State
        {
            int input_state = gpio_get_level(INPUT_PIN);
            gpio_set_level(LED, (input_state == 0) ? 1 : 0);
            //printf("PC Power State: %d\n", input_state);
            return input_state + 1;
        }
    }
}
