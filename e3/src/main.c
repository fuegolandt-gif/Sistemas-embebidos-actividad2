#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cJSON.h"


static void procesar_json(const char *mensaje)
{
    cJSON *root = cJSON_Parse(mensaje);

    if (root == NULL) {
        printf("Error: JSON es invalido\n");
        return;
    }

    cJSON *id   = cJSON_GetObjectItem(root, "ID");
    cJSON *temp = cJSON_GetObjectItem(root, "Temperatura");
    cJSON *hum  = cJSON_GetObjectItem(root, "Humedad");
    cJSON *dist = cJSON_GetObjectItem(root, "Distancia");

    if (!id || !temp || !hum || !dist) {
        printf("Error: JSON esta incompleto\n");
        cJSON_Delete(root);
        return;
    }

    if (!cJSON_IsString(id) ||
        !cJSON_IsNumber(temp) ||
        !cJSON_IsNumber(hum) ||
        !cJSON_IsNumber(dist)) {
        printf("Error: los tipos de datos son incorrectos\n");
        cJSON_Delete(root);
        return;
    }

    printf("\n------ DATOS VALIDOS ------\n");
    printf("ID: %s\n", id->valuestring);
    printf("la temperatura es: %.2f\n", temp->valuedouble);
    printf("la humedad es: %.2f\n", hum->valuedouble);
    printf("la distancia es: %.2f\n", dist->valuedouble);
    printf("---------------------------\n");

    cJSON_Delete(root);
}

void app_main(void)
{
    float temperatura;
    float humedad;
    int distancia;
    char json_buffer[200];

    // Inicializar el generador pseudoaleatorio
    srand(1234);   // semilla fija (se puede cambiar)

    while (1) {
        temperatura = 20.0f + (rand() % 1500) / 100.0f;  // 20.00 a 35.00
        humedad     = 40.0f + (rand() % 3000) / 100.0f;  // 40.00 a 70.00
        distancia   = 50 + (rand() % 200);              // 50 a 249

        sprintf(json_buffer,
            "{"
            "\"ID\":\"ESP32_01\","
            "\"Temperatura\":%.2f,"
            "\"Humedad\":%.2f,"
            "\"Distancia\":%d"
            "}",
            temperatura, humedad, distancia
        );

        printf("\nJSON generado:\n%s\n", json_buffer);
        procesar_json(json_buffer);

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}