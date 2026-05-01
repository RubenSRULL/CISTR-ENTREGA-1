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
static const char *TAG1 = "UNIDAD DE CARGA";
static const char *TAG2 = "UNIDAD DE PREPARACION";
static const char *TAG3 = "UNIDAD DE PROCESAMIENTO";
static const char *TAG4 = "PANEL DE ESTADO";

static int contador = 0;

// Variables globales para tiempos
uint32_t tiempoInicio = 0;
uint32_t tiempoFinEstacion1 = 0;
uint32_t tiempoFinEstacion2 = 0;
uint32_t tiempoFinEstacion3 = 0;

// Parametros tareas
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
                ESP_LOGI(TAG1, ">>> CEMENTO: %d | Elementos en cola: %d", valorAEnviar, itemsEnCola);
            }
            else
            {
                ESP_LOGI(TAG1, ">>> Cemento ya disponible");
            }
        }

        else if (boton_agua_state == 0)
        {
            char valorAEnviar = 1;
            if (xQueueSend(xQueueAgua, &valorAEnviar, 0) == pdPASS)
            {
                int itemsEnCola = uxQueueMessagesWaiting(xQueueAgua);
                ESP_LOGI(TAG1, ">>> AGUA: %d | Elementos en cola: %d", valorAEnviar, itemsEnCola);
            }
            else
            {
                ESP_LOGI(TAG1, ">>> Agua ya disponible");
            }
        }

        else if (boton_arena_state == 0)
        {
            char valorAEnviar = 1;
            if (xQueueSend(xQueueArena, &valorAEnviar, 0) == pdPASS)
            {
                int itemsEnCola = uxQueueMessagesWaiting(xQueueArena);
                ESP_LOGI(TAG1, ">>> ARENA: %d | Elementos en cola: %d", valorAEnviar, itemsEnCola);
            }
            else
            {
                ESP_LOGI(TAG1, ">>> Arena ya disponible");
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
            xQueueReceive(xQueueCemento, &material, portMAX_DELAY);
            xQueueReceive(xQueueAgua, &material, portMAX_DELAY);
            pack = 'S';
            enviar = 1;
        }

        else if (hayCemento && hayArena) {
            xQueueReceive(xQueueCemento, &material, portMAX_DELAY);
            xQueueReceive(xQueueArena, &material, portMAX_DELAY);
            pack = 'W';
            enviar = 1;
        }

        else if (hayArena && hayAgua) {
            xQueueReceive(xQueueAgua, &material, portMAX_DELAY);
            xQueueReceive(xQueueArena, &material, portMAX_DELAY);
            pack = 'C';
            enviar = 1;
        }

        if (enviar == 1)
        {
            if (xQueueSend(xQueuePacks, &pack, 0) == pdPASS)
            {
                enviar = 0;
                int itemsEnCola = uxQueueMessagesWaiting(xQueuePacks);
                tiempoInicio = xTaskGetTickCount() * portTICK_PERIOD_MS;
                ESP_LOGI(TAG2, ">>> ENVIADO [%u ms]: %c | Elementos en cola: %d",tiempoInicio, pack, itemsEnCola);
            }
            else
            {
                ESP_LOGW(TAG2, "Unidad de Preparación llena");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


// Tarea 3: Procesado
void procesado(void *pvParameters) {
    char pack;
    char valorAEnviar = 1;

    while (1)
    {
        xQueueReceive(xQueuePacks, &pack, portMAX_DELAY);

        if (pack == 'W')
        {
            xQueueSend(xQueueEstacion1, &valorAEnviar, 0);
            ESP_LOGI(TAG3, ">>> Pack %c enviado a estación 1", pack);
        }
        else if (pack == 'S')
        {
            xQueueSend(xQueueEstacion2, &valorAEnviar, 0);
            ESP_LOGI(TAG3, ">>> Pack %c enviado a estación 2", pack);
        }
        else if (pack == 'C')
        {
            xQueueSend(xQueueEstacion3, &valorAEnviar, 0);
            ESP_LOGI(TAG3, ">>> Pack %c enviado a estación 3", pack);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


// Tarea 4: Estacion1
void estacion1(void *pvParameters) {
    while (1)
    {
        char paqueteProcesado;
        xQueueReceive(xQueueEstacion1, &paqueteProcesado, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(10000));
        contador++;
        tiempoFinEstacion1 = xTaskGetTickCount() * portTICK_PERIOD_MS;
        int tiempoTranscurrido = tiempoFinEstacion1 - tiempoInicio;
        ESP_LOGI(TAG3, ">>> PACK PROCESADO en estación 1 [%d] (Total -> %d)", tiempoFinEstacion1, tiempoTranscurrido);
    }
}


// Tarea 5: Estacion2
void estacion2(void *pvParameters) {
    while (1)
    {
        char paqueteProcesado;
        xQueueReceive(xQueueEstacion2, &paqueteProcesado, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(10000));
        contador++;
        tiempoFinEstacion2 = xTaskGetTickCount() * portTICK_PERIOD_MS;
        int tiempoTranscurrido = tiempoFinEstacion2 - tiempoInicio;
        ESP_LOGI(TAG3, ">>> PACK PROCESADO en estación 2 [%d] (Total -> %d)", tiempoFinEstacion2, tiempoTranscurrido);
    }
}


// Tarea 6: Estacion3
void estacion3(void *pvParameters) {
    while (1)
    {
        char paqueteProcesado;
        xQueueReceive(xQueueEstacion3, &paqueteProcesado, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(10000));
        contador++;
        tiempoFinEstacion3 = xTaskGetTickCount() * portTICK_PERIOD_MS;
        int tiempoTranscurrido = tiempoFinEstacion3 - tiempoInicio;
        ESP_LOGI(TAG3, ">>> PACK PROCESADO en estación 3 [%d] (Total -> %d)", tiempoFinEstacion3, tiempoTranscurrido);
    }
}


// Tarea 7: Panel de estado
void panelEstado(void *pvParameters) {
    while (1) 
    {
        const char *cemento = (uxQueueMessagesWaiting(xQueueCemento) > 0) ? "Yes" : "No";
        const char *agua    = (uxQueueMessagesWaiting(xQueueAgua) > 0)    ? "Yes" : "No";
        const char *arena   = (uxQueueMessagesWaiting(xQueueArena) > 0)   ? "Yes" : "No";
        int packsEnEspera = uxQueueMessagesWaiting(xQueuePacks);

        ESP_LOGI(TAG4, 
            "\n--- PANEL DE ESTADO ---\n"
            "Estado de materias primas:\n"
            "  - Cemento: [%s]\n"
            "  - Agua:    [%s]\n"
            "  - Arena:   [%s]\n"
            "Packs en preparación: %u\n"
            "Packs procesados:     %d\n",
            cemento, agua, arena, packsEnEspera, contador
        );
        vTaskDelay(pdMS_TO_TICKS(2000));
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
        2,
        &pvCreatedTask,
        0
    );

    xTaskCreatePinnedToCore(
        preparacion,
        "Preparacion_Pack",
        usStackDepth,
        &pvParameters,
        3,
        &pvCreatedTask,
        0
    );

    xTaskCreatePinnedToCore(
        procesado,
        "Procesado_Pack",
        usStackDepth,
        &pvParameters,
        4,
        &pvCreatedTask,
        0
    );

    xTaskCreatePinnedToCore(
        estacion1,
        "Estacion1",
        usStackDepth,
        &pvParameters,
        5,
        &pvCreatedTask,
        0
    );

    xTaskCreatePinnedToCore(
        estacion2,
        "Estacion2",
        usStackDepth,
        &pvParameters,
        5,
        &pvCreatedTask,
        0
    );

    xTaskCreatePinnedToCore(
        estacion3,
        "Estacion3",
        usStackDepth,
        &pvParameters,
        5,
        &pvCreatedTask,
        0
    );

    xTaskCreatePinnedToCore(
        panelEstado,
        "PanelEstado",
        usStackDepth,
        &pvParameters,
        1,
        &pvCreatedTask,
        0
    );

}