// Importando as bibliotecas
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

// Definindo componentes
#define BUTTON GPIO_NUM_34
#define LED1 GPIO_NUM_21
#define LED2 GPIO_NUM_19
#define LED3 GPIO_NUM_18
#define LED4 GPIO_NUM_17

// Variável global
uint8_t v = 0;

void setup(){
  // Configurando LEDS como saindo
    gpio_config_t io_conf_leds = {
      .pin_bit_mask = (1ULL << LED1) | (1ULL << LED2) |(1ULL << LED3) |(1ULL << LED4),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_leds);

  // Configurando BUTTON como entrada
    gpio_config_t io_conf_button = {
      .pin_bit_mask = (1ULL << BUTTON),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_button);
}
 
void loop(){

  //varivel utilizada para pegar o nivel logico do button
  int estado_button = gpio_get_level(BUTTON);

  //set o nivel logico dos LEDS em 0 quando o button esta 0
  if(estado_button == 0){
    gpio_set_level(LED1,0);
    gpio_set_level(LED2,0);
    gpio_set_level(LED3,0);
    gpio_set_level(LED4,0);

    //variavel pro pseudo aleatorio
    v++;
    if(v>15){
      v = 0;
    }
    delay(500);
  }
  
  //set o nivel logico dos LEDs com base nos bits da variavel v
  else{
    gpio_set_level(LED1, v & 1);
    gpio_set_level(LED2, (v>>1) & 1);
    gpio_set_level(LED3, (v>>2) & 1);
    gpio_set_level(LED4, (v>>3) & 1);
   
   delay(500);
  }

  printf("v= %d\n",v);
}