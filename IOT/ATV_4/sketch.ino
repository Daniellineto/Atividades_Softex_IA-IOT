//Bibliotecas C
#include <stdio.h>
#include <stdbool.h>

//Bibliotecas Semáforo
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "freertos/semphr.h"

//Biblioteca ESP
#include "esp_system.h"
#include <esp_err.h>
#include "driver/gpio.h"

//Biblioteca ultrassônico
#include "ultrasonic.h"

// LEDS 
#define LED_RED GPIO_NUM_19
#define LED_YELLOW GPIO_NUM_18
#define LED_GREEN GPIO_NUM_5

#define GPIO_OUTPUT_PIN_SEL ((1ULL<<LED_RED) | (1ULL<<LED_YELLOW) | (1ULL<<LED_GREEN))

enum estados_leds {RED,YELLOW,GREEN}; 

static TaskHandle_t xtask_handle_leds = NULL; 
enum estados_leds state = RED;

// Pinos Ultrassônico 
#define TRIGGER_GPIO GPIO_NUM_12
#define ECHO_GPIO GPIO_NUM_14
#define MAX_DISTANCE_CM 500 // Maximum of 5 meters

void init_leds() {
  gpio_config_t led_conf = {};
  led_conf.intr_type = GPIO_INTR_DISABLE; 
  led_conf.mode = GPIO_MODE_OUTPUT; 
  led_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
  led_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; 
  led_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&led_conf);
}

int update_leds(enum estados_leds *state) {
  if(*state == RED) {
    gpio_set_level(LED_RED,1);
    gpio_set_level(LED_YELLOW,0);
    gpio_set_level(LED_GREEN,0);
    *state = GREEN;
    return 5;
  }
  else if(*state == GREEN) {
 
    gpio_set_level(LED_RED,0);
    gpio_set_level(LED_YELLOW,0);
    gpio_set_level(LED_GREEN,1);
    *state = YELLOW;
    return 10;
  }
  else {

    gpio_set_level(LED_RED,0);
    gpio_set_level(LED_YELLOW,1);
    gpio_set_level(LED_GREEN,0);
    *state = RED;
    return 1;
  }
}

void control_leds(void *pvParameters) {
  while(true){
    int time = update_leds(&state);
    vTaskDelay(pdMS_TO_TICKS(1000 * time));
  }
}

void ultrasonic_test(void *pvParameters) {
    float distance;
    ultrasonic_sensor_t sensor = {
        .trigger_pin = TRIGGER_GPIO,
        .echo_pin = ECHO_GPIO
    };

    ultrasonic_init(&sensor);
    int ALERTA = 0;
    
    while (true) {
        esp_err_t res = ultrasonic_measure(&sensor, MAX_DISTANCE_CM, &distance);

        if (res == ESP_OK) {
          printf("Distance: %0.04f m\n", distance); 

          if(distance <= 1 && ALERTA == 0){ 
            gpio_set_level(LED_RED,1);
            gpio_set_level(LED_YELLOW,0);
            gpio_set_level(LED_GREEN,0);
            
            vTaskSuspend(xtask_handle_leds);
            state = RED;
            ALERTA = 1;
          }
          else if(distance > 1 && ALERTA == 1){
            vTaskResume(xtask_handle_leds);
            ALERTA = 0;
          }
        } 

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void setup() {
    init_leds();
    xTaskCreate(ultrasonic_test, "ultrasonic_test", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    xTaskCreate(control_leds, "Semáforo", 2048, NULL, 5, &xtask_handle_leds);
}

void loop() {
    // Mantém vazio pois tudo está rodando nas tasks
    vTaskDelay(pdMS_TO_TICKS(1000)); // Apenas para reduzir uso da CPU
}
