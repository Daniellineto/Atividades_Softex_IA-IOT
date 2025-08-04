// Bibliotecas padrão
#include <stdio.h> 
#include <string.h>
#include <stdbool.h>

// Bibliotecas do FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

// GPIO e sensor ultrassônico
#include "driver/gpio.h"
#include "ultrasonic.h"

// Wi-Fi, sistema, logs e MQTT
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "mqtt_client.h"

// =============================== DEFINIÇÕES ==================================

// Pinos e parâmetros do sensor ultrassônico
#define TRIGGER_GPIO GPIO_NUM_12
#define ECHO_GPIO GPIO_NUM_14
#define MAX_DISTANCE_CM 500

// Credenciais Wi-Fi
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASS ""
#define CONNECTED_BIT BIT0

// Variáveis globais
static EventGroupHandle_t wifi_event_group;
static const char *TAG = "LOG_MQTT";
SemaphoreHandle_t xSemaphore;
esp_mqtt_client_handle_t client;
float distanceGlobal = 0.0;

// =============================== SENSOR ULTRASSÔNICO ========================

// Tarefa que realiza leituras do sensor a cada 2 segundos
void ultrasonic_test(void *pvParameters) {
    float distance;
    ultrasonic_sensor_t sensor = {
        .trigger_pin = TRIGGER_GPIO,
        .echo_pin = ECHO_GPIO
    };

    ultrasonic_init(&sensor);

    while (true) {
        esp_err_t res = ultrasonic_measure(&sensor, MAX_DISTANCE_CM, &distance);

        if (res == ESP_OK) {
            distanceGlobal = distance;

            if (xSemaphore != NULL) {
                xSemaphoreGive(xSemaphore);  // Sinaliza que há nova leitura
            }
        } else {
            printf("Erro na medição: %d\n", res);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// =============================== EVENTOS WI-FI ==============================

// Manipulador de eventos Wi-Fi (conexão, desconexão, IP)
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    }
}

// =============================== INICIALIZAÇÃO WI-FI ========================

// Inicializa a interface Wi-Fi em modo estação (STA)
static void initialise_wifi(void) {
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

// =============================== EVENTOS MQTT ===============================

// Manipulador de eventos MQTT (conexão, mensagem, erro, etc.)
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
    client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("Mensagem recebida: %.*s\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            break;
    }
}

// =============================== INICIALIZAÇÃO MQTT =========================

// Configura o cliente MQTT e inicia a conexão
static void mqtt_initialize(void) {
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = "mqtt://test.mosquitto.org";

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

// =============================== ENVIO DE MENSAGENS ========================

// Tarefa que envia a distância lida via MQTT
void send_messages(void *pvParam) {
    char tx_buffer[128];
    while (1) {
        if (xSemaphore != NULL && xSemaphoreTake(xSemaphore, pdMS_TO_TICKS(3000))) {
            float value = distanceGlobal;
            sprintf(tx_buffer, "%.2f", value);
            printf("Enviando a distância: %s m\n", tx_buffer);
            esp_mqtt_client_publish(client, "topic/distancia", tx_buffer, 0, 0, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

// =============================== SETUP E LOOP ==============================

void setup() {
    ESP_ERROR_CHECK(nvs_flash_init());              // Inicializa NVS (necessário para Wi-Fi)
    initialise_wifi();                              // Conecta ao Wi-Fi
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
    mqtt_initialize();                              // Inicializa MQTT

    xSemaphore = xSemaphoreCreateBinary();          // Cria semáforo para sincronizar tarefas

    xTaskCreate(ultrasonic_test, "ultrasonic_test", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    xTaskCreate(send_messages, "send_messages", 2048, NULL, 4, NULL);
}


void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}

// =============================== FIX DO LINKER PARA IPV6 ===================

// Define função vazia para evitar erro de link com IPv6
extern "C" void lwip_hook_ip6_input(void) {
    // Função vazia para evitar erro de link
}
