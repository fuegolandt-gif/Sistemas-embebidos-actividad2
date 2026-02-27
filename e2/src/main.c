/* Sistema de telemetría: Análisis y cálculo de ráfagas JSON-like por UART */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

#define PUERTO_UART UART_NUM_0
#define TAMANO_BUFFER 1024

// Función para inicializar el hardware UART
void inicializar_uart() {
    uart_config_t configuracion_uart = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(PUERTO_UART, TAMANO_BUFFER * 2, 0, 0, NULL, 0);
    uart_param_config(PUERTO_UART, &configuracion_uart);
    uart_set_pin(PUERTO_UART, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

/* * Función que valida estrictamente la trama y extrae el valor.
 * Retorna 1 si es perfectamente válida, o 0 si debe ser ignorada.
 */
int validar_y_extraer_caudal(const char* trama, int* valor_salida) {
    const char* formato_esperado = "{'caudal': ";
    int longitud_formato = 11; // Cantidad de caracteres del prefijo

    // 1. Verificamos que la trama empiece EXACTAMENTE con "{'caudal': "
    for (int i = 0; i < longitud_formato; i++) {
        if (trama[i] != formato_esperado[i]) {
            return 0; // Falso, el formato no coincide
        }
    }

    int indice = longitud_formato;
    int valor = 0;
    int tiene_numeros = 0;

    // 2. Extraemos los números hasta encontrar la llave de cierre
    while (trama[indice] != '\0' && trama[indice] != '}') {
        
        // Si el carácter es estrictamente un número del 0 al 9
        if (trama[indice] >= '0' && trama[indice] <= '9') {
            valor = (valor * 10) + (trama[indice] - '0');
            tiene_numeros = 1;
        } else {
            // Si encuentra un punto decimal (ej. 10.5), una letra o un espacio extra, se ignora
            return 0; 
        }
        indice++;
    }

    // 3. Verificamos que termine correctamente con la llave '}'
    if (trama[indice] != '}') {
        return 0;
    }

    // 4. Aseguramos que realmente había un número dentro (evita "{'caudal': }")
    if (!tiene_numeros) {
        return 0;
    }

    // 5. Validamos el rango permitido (0 a 99)
    if (valor < 0 || valor > 99) {
        return 0;
    }

    // Pasó todos los filtros, pasamos el valor y confirmamos el éxito
    *valor_salida = valor;
    return 1;
}

void app_main() {
    inicializar_uart();

    // Variables de estado persistentes durante toda la ejecución
    int mayor = -1;
    int menor = -1; // Iniciamos en -1 para indicar que están vacíos al principio
    long suma_total = 0;
    int contador_datos = 0;

    char buffer[100];
    int indice = 0;

    while (1) {
        uint8_t caracter_recibido;
        // Leemos el puerto serial constantemente
        int longitud = uart_read_bytes(PUERTO_UART, &caracter_recibido, 1, pdMS_TO_TICKS(10));

        if (longitud > 0) {
            // Asumimos que el caudalímetro envía un salto de línea al terminar su mensaje
            if (caracter_recibido == '\n' || caracter_recibido == '\r') {
                if (indice > 0) {
                    buffer[indice] = '\0'; // Cerramos la cadena recibida

                    int valor_caudal = 0;
                    
                    // Solo si la trama es 100% válida y cumple el formato, hacemos el procesamiento
                    if (validar_y_extraer_caudal(buffer, &valor_caudal)) {
                        
                        // Lógica de persistencia de datos
                        if (contador_datos == 0) {
                            mayor = valor_caudal;
                            menor = valor_caudal;
                        } else {
                            if (valor_caudal > mayor) mayor = valor_caudal;
                            if (valor_caudal < menor) menor = valor_caudal;
                        }

                        suma_total += valor_caudal;
                        contador_datos++;
                        float promedio = (float)suma_total / contador_datos;

                        // Armamos la trama de respuesta solicitada
                        char respuesta[150];
                        snprintf(respuesta, sizeof(respuesta), 
                                 "{'último': %d, 'mayor': %d, 'menor': %d, 'promedio': %.2f}\n", 
                                 valor_caudal, mayor, menor, promedio);
                        
                        // Enviamos la respuesta por el UART
                        uart_write_bytes(PUERTO_UART, respuesta, strlen(respuesta));
                    }
                    // Si la función de validación devolvió 0 (falso), el programa simplemente 
                    // ignora el bloque if, no calcula nada, no imprime nada, y sigue adelante.

                    // Limpiamos el índice para recibir la próxima ráfaga del caudalímetro
                    indice = 0; 
                }
            } else {
                // Mientras no llegue el salto de línea, seguimos acumulando los caracteres
                if (indice < sizeof(buffer) - 1) {
                    buffer[indice] = (char)caracter_recibido;
                    indice++;
                }
            }
        }
    }
}