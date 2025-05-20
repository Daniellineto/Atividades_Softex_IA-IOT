// Importando as bibliotecas
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

// Definindo pinos
#define BUTTON_LEFT GPIO_NUM_27
#define BUTTON_RIGHT GPIO_NUM_26

#define LED1 GPIO_NUM_19
#define LED2 GPIO_NUM_18
#define LED3 GPIO_NUM_5
#define LED4 GPIO_NUM_17
#define LED5 GPIO_NUM_16
#define LED6 GPIO_NUM_4
#define LED7 GPIO_NUM_0
#define LED8 GPIO_NUM_2

// Função de interupção
static void IRAM_ATTR gpio0_isr_handler(void* arg){
  uint8_t *v = (uint8_t*) arg;
  *v = 0;
}

static void IRAM_ATTR gpio1_isr_handler(void* arg){
  uint8_t *v = (uint8_t*) arg;
  *v = 1;
}

// Variavel global que indica a direção
volatile uint8_t dir = 0;

// Configurando os pinos
 void setup(){
  
  // Configurando LEDS como saindo
    gpio_config_t io_conf_leds = {
      .pin_bit_mask = (1ULL << LED1) | (1ULL << LED2) | (1ULL << LED3) | (1ULL << LED4) |(1ULL << LED5) | (1ULL << LED6) | (1ULL << LED7) | (1ULL << LED8),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = 0,
      .pull_down_en = 0,
      .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_leds);

    // Configurando BUTTON como entrada
    gpio_config_t io_conf_button = {
      .pin_bit_mask = (1ULL << BUTTON_LEFT) | (1ULL << BUTTON_RIGHT) ,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = 0,
      .pull_down_en = 0,
      .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&io_conf_button);
 }

void app_main() {
  // Chama a config
  setup();

  // Variavel que armazena qual LED vai acender com base no numero
  uint8_t valor = 1;

  // Instala a interrupção
  gpio_install_isr_service(0);

  // Função para gerar interrupção
  gpio_isr_handler_add(BUTTON_LEFT, gpio0_isr_handler, (void*) &dir);
  gpio_isr_handler_add(BUTTON_RIGHT, gpio1_isr_handler, (void*) &dir);

  // Lista com os led
  gpio_num_t leds[8] = {LED1, LED2, LED3, LED4, LED5, LED6, LED7, LED8};

  while(true){

  // for para acender os LEDs com base no numero
  for (int i = 0; i < 8; i++) {
    gpio_set_level(leds[i], (valor >> i) & 1);
  }
    printf("Valor atual: %d\n", valor);
   
    // Para quando ele ta indo pra Esquerda (0) ou Direita (1)
    if (dir == 0){
      valor <<= 1;
      /* OBS:. o valor precisou ser == 0 pois com a variavel valor sendo uint8_t 
      é impossivel dar um valor maior que 128, quando a variavel incrementa 
      ela se torna 0 novamente*/
      if(valor == 0){
        valor = 1;
      }
    }
    
    else{
      valor >>= 1; 

      if (valor < 1){ 
        valor = 128;
      }
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  } 
}
