// Autor : Rubén Sahuquillo Redondo
// Entregable 1 CISTR - Sistema de control de producción de cemento

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

// Contador de packs procesados
static int contador = 0;

// Variables globales para disponibilidad de estaciones
static int disponible1 = 1;
static int disponible2 = 1;
static int disponible3 = 1;

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
    while (1) {
        if (gpio_get_level(PULSADOR_CEMENTO) == 0) {
            char valorAEnviar = 1;
            if (xQueueSend(xQueueCemento, &valorAEnviar, 0) == pdPASS) {
                ESP_LOGI(TAG1, ">>> CEMENTO cargado");
            }
            else {
                ESP_LOGW(TAG1, ">>> Unidad de Carga llena: No se pudo cargar cemento");
            }
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        else if (gpio_get_level(PULSADOR_AGUA) == 0) {
            char valorAEnviar = 1;
            if (xQueueSend(xQueueAgua, &valorAEnviar, 0) == pdPASS) {
                ESP_LOGI(TAG1, ">>> AGUA cargada");
            }
            else {
                ESP_LOGW(TAG1, ">>> Unidad de Carga llena: No se pudo cargar agua");
            }
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        else if (gpio_get_level(PULSADOR_ARENA) == 0) {
            char valorAEnviar = 1;
            if (xQueueSend(xQueueArena, &valorAEnviar, 0) == pdPASS) {
                ESP_LOGI(TAG1, ">>> ARENA cargada");
            }
            else {
                ESP_LOGW(TAG1, ">>> Unidad de Carga llena: No se pudo cargar arena");
            }
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        vTaskDelay(pdMS_TO_TICKS(50)); 
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
        if (xQueuePeek(xQueuePacks, &pack, portMAX_DELAY) == pdPASS)
        {
            int enviado = 0;

            if (pack == 'W' && disponible1 == 1)
            {
                if (xQueueSend(xQueueEstacion1, &valorAEnviar, 0) == pdPASS)
                {
                    xQueueReceive(xQueuePacks, &pack, 0);
                    ESP_LOGI(TAG3, ">>> Pack %c enviado a estación 1", pack);
                    enviado = 1;
                }
            }
            else if (pack == 'S' && disponible2 == 1)
            {
                if (xQueueSend(xQueueEstacion2, &valorAEnviar, 0) == pdPASS)
                {
                    xQueueReceive(xQueuePacks, &pack, 0);
                    ESP_LOGI(TAG3, ">>> Pack %c enviado a estación 2", pack);
                    enviado = 1;
                }
            }
            else if (pack == 'C' && disponible3 == 1)
            {
                if (xQueueSend(xQueueEstacion3, &valorAEnviar, 0) == pdPASS)
                {
                    xQueueReceive(xQueuePacks, &pack, 0);
                    ESP_LOGI(TAG3, ">>> Pack %c enviado a estación 3", pack);
                    enviado = 1;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Tareas 4, 5 y 6: Estaciones de procesamiento
void estacion1(void *pvParameters) {
    char paqueteProcesado;
    while (1) {
        if ((xQueuePeek(xQueueEstacion1, &paqueteProcesado, portMAX_DELAY)) && (disponible1 == 1)) {
            disponible1 = 0;
            vTaskDelay(pdMS_TO_TICKS(10000)); 
            xQueueReceive(xQueueEstacion1, &paqueteProcesado, 0); 
            contador++;
            uint32_t ahora = xTaskGetTickCount() * portTICK_PERIOD_MS;
            ESP_LOGI(TAG3, ">>> PACK Estación 1 listo. Total: %u ms", (unsigned int)(ahora - tiempoInicio));
            disponible1 = 1;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void estacion2(void *pvParameters) {
    char paqueteProcesado;
    while (1) {
        if ((xQueuePeek(xQueueEstacion2, &paqueteProcesado, portMAX_DELAY)) && (disponible2 == 1)) {
            disponible2 = 0;
            vTaskDelay(pdMS_TO_TICKS(10000)); 
            xQueueReceive(xQueueEstacion2, &paqueteProcesado, 0); 
            contador++;
            uint32_t ahora = xTaskGetTickCount() * portTICK_PERIOD_MS;
            ESP_LOGI(TAG3, ">>> PACK Estación 2 listo. Total: %u ms", (unsigned int)(ahora - tiempoInicio));
            disponible2 = 1;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void estacion3(void *pvParameters) {
    char paqueteProcesado;
    while (1) {
        if ((xQueuePeek(xQueueEstacion3, &paqueteProcesado, portMAX_DELAY)) && (disponible3 == 1)) {
            disponible3 = 0;
            vTaskDelay(pdMS_TO_TICKS(10000)); 
            xQueueReceive(xQueueEstacion3, &paqueteProcesado, 0); 
            contador++;
            uint32_t ahora = xTaskGetTickCount() * portTICK_PERIOD_MS;
            ESP_LOGI(TAG3, ">>> PACK Estación 3 listo. Total: %u ms", (unsigned int)(ahora - tiempoInicio));
            disponible3 = 1;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


// Tarea 7: Panel de estado
void panelEstado(void *pvParameters) {
    while (1) 
    {
        const char *cemento = (uxQueueMessagesWaiting(xQueueCemento) > 0) ? "Yes" : "No";
        const char *agua    = (uxQueueMessagesWaiting(xQueueAgua) > 0)    ? "Yes" : "No";
        const char *arena   = (uxQueueMessagesWaiting(xQueueArena) > 0)   ? "Yes" : "No";
        const char *cestacion1 = (disponible1 == 1) ? "Disponible" : "Ocupada";
        const char *cestacion2 = (disponible2 == 1) ? "Disponible" : "Ocupada";
        const char *cestacion3 = (disponible3 == 1) ? "Disponible" : "Ocupada";
        int packsEnEspera = uxQueueMessagesWaiting(xQueuePacks);

        ESP_LOGI(TAG4, 
            "\n--- PANEL DE ESTADO ---\n"
            "Estado de materias primas:\n"
            "  - Cemento: [%s]\n"
            "  - Agua:    [%s]\n"
            "  - Arena:   [%s]\n"
            "Packs almacenados en preparación: %d\n"
            "Estado de estaciones de procesamiento:\n"
            "  - Estación 1: [%s]\n"
            "  - Estación 2: [%s]\n"
            "  - Estación 3: [%s]\n"
            "Packs procesados:     %d\n",
            cemento, agua, arena, packsEnEspera, cestacion1, cestacion2, cestacion3, contador
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
    xQueueCemento = xQueueCreate(1, sizeof(char));
    xQueueAgua = xQueueCreate(1, sizeof(char));
    xQueueArena = xQueueCreate(1, sizeof(char));
    xQueuePacks= xQueueCreate(3, sizeof(char));
    xQueueEstacion1 = xQueueCreate(1, sizeof(char));
    xQueueEstacion2 = xQueueCreate(1, sizeof(char));
    xQueueEstacion3= xQueueCreate(1, sizeof(char));

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