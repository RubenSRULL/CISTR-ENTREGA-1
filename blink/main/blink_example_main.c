// Librerias
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

// Colas
QueueHandle_t xQueuePreparationZone;
QueueHandle_t xQueueStation1;
QueueHandle_t xQueueStation2;
QueueHandle_t xQueueStation3;

// Semaforos
SemaphoreHandle_t mutex;

// TAG para debug
static const char *TAG = "example";

// Parametros tares
static uint32_t usStackDepth = 2048;
TaskHandle_t pvCreatedTask = NULL;
TaskHandle_t pvParameters = NULL;


// Pulsadores de entrada
#define PULSADOR_CEMENTO GPIO_NUM_16
#define PULSADOR_AGUA GPIO_NUM_17
#define PULSADOR_ARENA GPIO_NUM_18


// Variables
static uint8_t cemento = 0;
static uint8_t arena = 0;
static uint8_t agua = 0;

// Funciones
const char* read_buttons()
{
    if (gpio_get_level(PULSADOR_CEMENTO) == 0)
    {
        return "CEMENTO";
    }

    if (gpio_get_level(PULSADOR_AGUA) == 0)
    {
        return "AGUA";
    }

    if (gpio_get_level(PULSADOR_ARENA) == 0)
    {
        return "ARENA";
    }

    return "NINGUNO";
}


// Tarea 1
void vTask1(void *pvParameters) {
    const char *material_actual;

    while (1) {
        int boton_cemento_state = gpio_get_level(PULSADOR_CEMENTO);
        int boton_agua_state = gpio_get_level(PULSADOR_AGUA);
        int boton_arena_state = gpio_get_level(PULSADOR_ARENA);
        int boton_cemento_last_state = 1;
        int boton_agua_last_state = 1;
        int boton_arena_last_state = 1;

        if ((boton_cemento_state == 0) && (boton_cemento_last_state == 1))
        {
            cemento ++;
            boton_cemento_last_state = boton_cemento_state;
            ESP_LOGI(TAG, "Cemento depositado (%d)", cemento);
        }
        if ((boton_agua_state == 0) && (boton_agua_last_state == 1))
        {
            agua++;
            boton_agua_last_state = boton_agua_state;
            ESP_LOGI(TAG, "Agua depositado (%d)", agua);
        }
        if ((boton_arena_state == 0) && (boton_arena_last_state == 1))
        {
            arena++;
            boton_arena_last_state = boton_cemento_state;
            ESP_LOGI(TAG, "Arena depositado (%d)", arena);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}


// Funcion para configurar perifericos
static void configure_peripherals(void)
{
    gpio_reset_pin(PULSADOR_CEMENTO);
    gpio_set_direction(PULSADOR_CEMENTO, GPIO_MODE_INPUT);
    gpio_pullup_en(PULSADOR_CEMENTO);
    gpio_pulldown_dis(PULSADOR_CEMENTO);

    gpio_reset_pin(PULSADOR_AGUA);
    gpio_set_direction(PULSADOR_AGUA, GPIO_MODE_INPUT);
    gpio_pullup_en(PULSADOR_AGUA);
    gpio_pulldown_dis(PULSADOR_AGUA);

    gpio_reset_pin(PULSADOR_ARENA);
    gpio_set_direction(PULSADOR_ARENA, GPIO_MODE_INPUT);
    gpio_pullup_en(PULSADOR_ARENA);
    gpio_pulldown_dis(PULSADOR_ARENA);
}


// Funcion con main
void app_main(void)
{
    // Configurar perifericos
    configure_peripherals();

    // Tareas
    xTaskCreatePinnedToCore(
        vTask1,
        "Tarea1",
        usStackDepth,
        &pvParameters,
        12,
        &pvCreatedTask,
        0
    );
}