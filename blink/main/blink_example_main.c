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
QueueHandle_t xQueueCemento;
QueueHandle_t xQueueAgua;
QueueHandle_t xQueueArena;
QueueHandle_t xQueuePacks;
QueueHandle_t xQueueEstacion1;
QueueHandle_t xQueueEstacion2;
QueueHandle_t xQueueEstacion3;


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


// Tarea 1: Carga
void carga(void *pvParameters) {
    while (1)
    {
        int boton_cemento_state = gpio_get_level(PULSADOR_CEMENTO);
        int boton_agua_state = gpio_get_level(PULSADOR_AGUA);
        int boton_arena_state = gpio_get_level(PULSADOR_ARENA);

        if (boton_cemento_state == 0)
        {
            char valorAEnviar = 1;
            if (xQueueSend(xQueueCemento, &valorAEnviar, 0) == pdPASS)
            {
                int itemsEnCola = uxQueueMessagesWaiting(xQueueCemento);
                ESP_LOGI(TAG, ">>> CEMENTO: %d | Elementos en cola: %d", valorAEnviar, itemsEnCola);
            }
            else
            {
                ESP_LOGI(TAG, ">>> Estación de cemento llena");
            }
        }

        if (boton_agua_state == 0)
        {
            char valorAEnviar = 1;
            if (xQueueSend(xQueueAgua, &valorAEnviar, 0) == pdPASS)
            {
                int itemsEnCola = uxQueueMessagesWaiting(xQueueAgua);
                ESP_LOGI(TAG, ">>> AGUA: %d | Elementos en cola: %d", valorAEnviar, itemsEnCola);
            }
            else
            {
                ESP_LOGI(TAG, ">>> Estación de agua llena");
            }
        }

        if (boton_arena_state == 0)
        {
            char valorAEnviar = 1;
            if (xQueueSend(xQueueArena, &valorAEnviar, 0) == pdPASS)
            {
                int itemsEnCola = uxQueueMessagesWaiting(xQueueArena);
                ESP_LOGI(TAG, ">>> ARENA: %d | Elementos en cola: %d", valorAEnviar, itemsEnCola);
            }
            else
            {
                ESP_LOGI(TAG, ">>> Estación de arena llena");
            }
        }

    vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}


// Tarea 2: Preparacion
void preparacion(void *pvParameters) {
    char material;
    
    while (1) {
        int hayCemento = uxQueueMessagesWaiting(xQueueCemento) > 0;
        int hayAgua    = uxQueueMessagesWaiting(xQueueAgua) > 0;
        int hayArena   = uxQueueMessagesWaiting(xQueueArena) > 0;
        char pack;
        int enviar = 0;

        if (hayCemento && hayAgua) {
            xQueueReceive(xQueueCemento, &material, 0);
            xQueueReceive(xQueueAgua, &material, 0);
            pack = 'S';
            enviar = 1;
        }

        else if (hayCemento && hayArena) {
            xQueueReceive(xQueueCemento, &material, 0);
            xQueueReceive(xQueueArena, &material, 0);
            pack = 'W';
            enviar = 1;
        }

        else if (hayArena && hayAgua) {
            xQueueReceive(xQueueAgua, &material, 0);
            xQueueReceive(xQueueArena, &material, 0); 
            pack = 'C';
            enviar = 1;
        }

        if (enviar == 1)
        {
            if (xQueueSend(xQueuePacks, &pack, 0) == pdPASS)
            {
                enviar = 0;
                int itemsEnCola = uxQueueMessagesWaiting(xQueuePacks);
                ESP_LOGI(TAG, ">>> ENVIADO: %c | Elementos en cola: %d", pack, itemsEnCola);
            }
            else
            {
                ESP_LOGI(TAG, "Cola de packs llena");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


// Tarea 3: Procesado
void procesado(void *pvParameters) {
    int enviar = 0;
    char pack;
    char valorAEnviar = 1;

    while (1)
    {
        int hayPack = uxQueueMessagesWaiting(xQueuePacks) > 0;
        if (hayPack)
        {
            xQueueReceive(xQueuePacks, &pack, 0);
            enviar = 1;
        }

        if (enviar == 1)
        {
            if (pack == 'W')
            {
                if (xQueueSend(xQueueEstacion1, &valorAEnviar, 0) == pdPASS)
                {
                    enviar = 0;
                    int itemsEnCola = uxQueueMessagesWaiting(xQueueEstacion1);
                    ESP_LOGI(TAG, ">>> PACK ENVIADO a estación 1 (agua) | Elementos en cola: %d", itemsEnCola);
                    vTaskDelay(pdMS_TO_TICKS(10000));
                    char paqueteProcesado;
                    xQueueReceive(xQueueEstacion1, &paqueteProcesado, 0);
                    ESP_LOGI(TAG, ">>> PACK PROCESADO en estación 1 (agua) | Elementos en cola: %d", uxQueueMessagesWaiting(xQueueEstacion1));

                }
                else
                {
                    ESP_LOGI(TAG, "Cola de Estacion 1 llena");
                }
            }

            else if (pack == 'S')
            {
                if (xQueueSend(xQueueEstacion2, &valorAEnviar, 0) == pdPASS)
                {
                    enviar = 0;
                    int itemsEnCola = uxQueueMessagesWaiting(xQueueEstacion2);
                    ESP_LOGI(TAG, ">>> PACK ENVIADO a estación 2 (arena) | Elementos en cola: %d", itemsEnCola);
                    vTaskDelay(pdMS_TO_TICKS(10000));
                    char paqueteProcesado;
                    xQueueReceive(xQueueEstacion2, &paqueteProcesado, 0);
                    ESP_LOGI(TAG, ">>> PACK PROCESADO en estación 2 (arena) | Elementos en cola: %d", uxQueueMessagesWaiting(xQueueEstacion2));
                }
                else
                {
                    ESP_LOGI(TAG, "Cola de Estacion 2 llena");
                }
            }

            else if (pack == 'C')
            {
                if (xQueueSend(xQueueEstacion3, &valorAEnviar, 0) == pdPASS)
                {
                    enviar = 0;
                    int itemsEnCola = uxQueueMessagesWaiting(xQueueEstacion3);
                    ESP_LOGI(TAG, ">>> PACK ENVIADO a estación 3 (cemento) | Elementos en cola: %d", itemsEnCola);
                    vTaskDelay(pdMS_TO_TICKS(10000));
                    char paqueteProcesado;
                    xQueueReceive(xQueueEstacion3, &paqueteProcesado, 0);
                    ESP_LOGI(TAG, ">>> PACK PROCESADO en estación 1 (cemento) | Elementos en cola: %d", uxQueueMessagesWaiting(xQueueEstacion3));
                }
                else
                {
                    ESP_LOGI(TAG, "Cola de Estacion 3 llena");
                }
            }  
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

    // Colas
    xQueueCemento = xQueueCreate(1, sizeof(int));
    xQueueAgua = xQueueCreate(1, sizeof(int));
    xQueueArena = xQueueCreate(1, sizeof(int));
    xQueuePacks= xQueueCreate(3, sizeof(int));
    xQueueEstacion1 = xQueueCreate(1, sizeof(int));
    xQueueEstacion2 = xQueueCreate(1, sizeof(int));
    xQueueEstacion3= xQueueCreate(3, sizeof(int));

    // Tareas
    xTaskCreatePinnedToCore(
        carga,
        "Carga_Materiales",
        usStackDepth,
        &pvParameters,
        12,
        &pvCreatedTask,
        0
    );

    xTaskCreatePinnedToCore(
        preparacion,
        "Preparacion_Pack",
        usStackDepth,
        &pvParameters,
        12,
        &pvCreatedTask,
        0
    );

    xTaskCreatePinnedToCore(
        procesado,
        "Procesado_Pack",
        usStackDepth,
        &pvParameters,
        12,
        &pvCreatedTask,
        0
    );

    
}