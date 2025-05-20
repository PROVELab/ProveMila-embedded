#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//define GPIO pin numbers
#define INPUT_PIN 9   //car ON switch
#define CONTACTOR_1 10   //RL1
#define CONTACTOR_2 11   //RL2
#define CONTACTOR_3 12   //RL3
#define LED_PIN 13   //LED indicator

//precharge circuit code used for testing. Not intended for final design (will also be turned on/off via can messages)
//make sure to use a switch for input 9 when using this code to test, otherwise the value (and hence code) is undefined

void app_main(void) {
  //configure input pin
  gpio_config_t input_conf;
  input_conf.pin_bit_mask = (1ULL << INPUT_PIN);
  input_conf.mode = GPIO_MODE_INPUT;
  input_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  input_conf.pull_up_en = GPIO_PULLUP_ENABLE; //pull-up to avoid floating
  input_conf.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&input_conf);

  //configure output pins
  gpio_config_t output_conf;
  output_conf.pin_bit_mask = (1ULL << CONTACTOR_1) | (1ULL << CONTACTOR_2) | (1ULL << CONTACTOR_3) | (1ULL << LED_PIN);
  output_conf.mode = GPIO_MODE_OUTPUT;
  output_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  output_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  output_conf.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&output_conf);

  //initialize outputs to LOW
  gpio_set_level((gpio_num_t)CONTACTOR_1, 0);
  gpio_set_level((gpio_num_t)CONTACTOR_2, 0);
  gpio_set_level((gpio_num_t)CONTACTOR_3, 0);
  gpio_set_level((gpio_num_t)LED_PIN, 0);

  while (1) {
    if (gpio_get_level((gpio_num_t)INPUT_PIN)) {
      //car is ON — begin precharge
      gpio_set_level((gpio_num_t)CONTACTOR_2, 1);  //close RL2
      gpio_set_level((gpio_num_t)CONTACTOR_3, 1);  //close RL3
      gpio_set_level((gpio_num_t)LED_PIN, 1);      //LED on during precharge

      vTaskDelay(pdMS_TO_TICKS(5000)); //5 sec delay

      gpio_set_level((gpio_num_t)LED_PIN, 0);      //LED off

      //after precharge, keep RL1 closed and RL3 open while car is ON
      while (gpio_get_level((gpio_num_t)INPUT_PIN)) {
        gpio_set_level((gpio_num_t)CONTACTOR_1, 1); //close RL1
        gpio_set_level((gpio_num_t)CONTACTOR_3, 0); //open RL3
        vTaskDelay(pdMS_TO_TICKS(100)); //poll delay
      }

      //car turned off — reset everything
      gpio_set_level((gpio_num_t)CONTACTOR_1, 0);
      gpio_set_level((gpio_num_t)CONTACTOR_2, 0);
      gpio_set_level((gpio_num_t)CONTACTOR_3, 0);
    }

    vTaskDelay(pdMS_TO_TICKS(100)); //polling interval
  }
}
