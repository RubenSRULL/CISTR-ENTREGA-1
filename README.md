# CISTR Entregable 1: Control de Planta de Fabricación de Cemento

Este proyecto consiste en el diseño y programación del software de control para un proceso de fabricación industrial de cemento en tiempo real, utilizando un microcontrolador **ESP32** y el framework **ESP-IDF**.

Desarrollado para la asignatura de *Comunicaciones Industriales y Sistemas en Tiempo Real* (Curso 2025/2026).


## Descripción del Proceso

El sistema simula una planta industrial automatizada con tres unidades principales:

1. **Unidad de Carga**: Gestiona la entrada de materias primas (Cemento, Arena y Agua) mediante pulsadores.
2. **Unidad de Preparación**: Combina las materias primas para formar "packs" de dos componentes. Estos se almacenan en una cola **FIFO** con una capacidad máxima de **3 packs**.
3. **Unidad de Procesado**:
   - **Opción Avanzada I**: Las tres estaciones (Agua, Arena, Cemento) operan de forma simultánea e independiente, reduciendo los tiempos de espera.
   - Cada proceso de mezcla tiene una duración determinista de **10 segundos**.

## Especificaciones Técnicas

- **Comunicación entre tareas**: Colas de mensajes (`vQueue`).
- **Monitoreo**: Panel de estado por consola cada 2 segundos y registro de eventos en tiempo real.
- **Hardware**: ESP32.
- **Framework**: ESP-IDF (Espressif IoT Development Framework).


## Instalación

1. Clona o descarga este repositorio en tu máquina local.
2. Navega al directorio del proyecto: `cd CISTR-Entregable-1`.
3. Configura el entorno ESP-IDF ejecutando `idf.py set-target esp32` (o la variante de ESP32 que uses).

# Build y flash
 - Limpiar carpeta build y compilar
 - flasheo del proyecto en puerto correcto


## Estructura del Proyecto

```
CISTR Entregable 1/
│
├── README.md                             # Este archivo
├── blink/                                # Directorio principal del proyecto ESP-IDF
│   ├── CMakeLists.txt                    # Configuración de CMake para el proyecto
│   ├── sdkconfig                         # Configuración de hardware/FreeRTOS
│   ├── sdkconfig.defaults                # Configuraciones por defecto
│   ├── sdkconfig.defaults.esp32          # Configuración específica para ESP32
│   ├── ... (otros archivos defaults)     # Configuraciones para otras variantes
│   ├── build/                            # Binarios generados (no versionado)
│   ├── main/                             # Código fuente principal
│   │   ├── blink_example_main.c          # Lógica principal, tareas y colas
│   │   ├── CMakeLists.txt                # Registro de fuentes
│   │   ├── idf_component.yml             # Configuración de componentes
│   │   └── Kconfig.projbuild             # Configuración de Kconfig
│   └── managed_components/               # Componentes gestionados
│       └── espressif__led_strip/         # Componente para control de LEDs
├── doc/                                  # Documentación
│   └── DIAGRAMA PROCESO PROGRAMA.drawio  # Diagrama del proceso
```