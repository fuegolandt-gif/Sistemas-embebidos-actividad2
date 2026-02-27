/* Calcular el cuadrado sumando N impares usando el driver UART de ESP-IDF */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h" // Librería fundamental para usar el UART directamente

#define PUERTO_UART UART_NUM_0 // Usamos el UART0 (el predeterminado para el cable USB)
#define TAMANO_BUFFER 1024

// Función matemática (sin cambios)
long long calcularCuadrado(int n) {
    long long suma = 0;
    long long impar_actual = 1; 

    for (int i = 0; i < n; i++) {
        suma += impar_actual;  
        impar_actual += 2;     
    }
    return suma;
}

// Función para configurar e inicializar el UART
void inicializar_uart() {
    uart_config_t configuracion_uart = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // 1. Instalamos el driver en el puerto 0
    uart_driver_install(PUERTO_UART, TAMANO_BUFFER * 2, 0, 0, NULL, 0);
    // 2. Aplicamos la configuración de velocidad y bits
    uart_param_config(PUERTO_UART, &configuracion_uart);
    // 3. Asignamos los pines (UART_PIN_NO_CHANGE usa los pines TX/RX por defecto de la placa)
    uart_set_pin(PUERTO_UART, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void app_main() {
    // Inicializamos nuestro hardware UART primero
    inicializar_uart();
    vTaskDelay(pdMS_TO_TICKS(2000)); 

    // Para enviar texto por UART, primero lo guardamos en una variable
    const char* mensaje_bienvenida = "\n=========================================\n  Calculadora    \n=========================================\n";
    // Y luego lo transmitimos indicando la longitud del texto
    uart_write_bytes(PUERTO_UART, mensaje_bienvenida, strlen(mensaje_bienvenida));

    while (1) {
        const char* mensaje_pedir = "\nIngresa un numero (del 1 al 9999) y presiona Enter: ";
        uart_write_bytes(PUERTO_UART, mensaje_pedir, strlen(mensaje_pedir));

        char buffer[10]; 
        int indice = 0;
        int entrada_lista = 0;
        int entrada_invalida = 0; 

        while (!entrada_lista) {
            uint8_t caracter_recibido;
            
            // Leemos exactamente 1 byte (carácter) desde el buffer del UART. 
            // Esperamos un máximo de 10ms (pdMS_TO_TICKS(10)) por si no hay nada escrito.
            int longitud = uart_read_bytes(PUERTO_UART, &caracter_recibido, 1, pdMS_TO_TICKS(10));

            // Si longitud > 0, significa que el usuario presionó una tecla
            if (longitud > 0) {
                if (caracter_recibido == '\n' || caracter_recibido == '\r') {
                    if (indice > 0) { 
                        buffer[indice] = '\0'; 
                        entrada_lista = 1;
                    }
                } 
                else if (caracter_recibido >= '0' && caracter_recibido <= '9') {
                    if (indice < 4) {
                        // Hacemos el "eco" devolviendo el mismo byte por el TX del UART
                        uart_write_bytes(PUERTO_UART, (const char*)&caracter_recibido, 1); 
                        buffer[indice] = (char)caracter_recibido;
                        indice++;
                    }
                } 
                else {
                    entrada_invalida = 1;
                }
            }
        }

        const char* salto_linea = "\n";
        uart_write_bytes(PUERTO_UART, salto_linea, strlen(salto_linea));

        int numero_ingresado = atoi(buffer);
        char respuesta_formateada[150]; // Creamos un buffer de texto para armar la respuesta

        if (entrada_invalida == 0 && numero_ingresado >= 1 && numero_ingresado <= 9999) {
            long long resultado = calcularCuadrado(numero_ingresado);
            
            // Usamos snprintf para "armar" el texto de la respuesta con las variables
            snprintf(respuesta_formateada, sizeof(respuesta_formateada), 
                     " -> Numero recibido: %d\n -> El cuadrado (sumando %d impares) es: %lld\n", 
                     numero_ingresado, numero_ingresado, resultado);
        } else {
            snprintf(respuesta_formateada, sizeof(respuesta_formateada), 
                     " -> [Entrada ignorada] Por favor, ingresa un numero valido entre 1 y 9999.\n");
        }

        // Finalmente, enviamos la respuesta armada por el UART
        uart_write_bytes(PUERTO_UART, respuesta_formateada, strlen(respuesta_formateada));

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}