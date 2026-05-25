#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_log.h>
#include <mpu6050.h>
#include <math.h>
#include "config.h"
#include "app_container/mpu6050.h"
#include "app_container/mqtt_publisher.h"
#include "mqtt_client.h"
#include "app_container/ble_server.h"

#define REAL_MPU_ADDR MPU6050_I2C_ADDRESS_LOW 

// Definições da Janela e Frequência
#define FREQ_HZ          18
#define WINDOW_DURATION  2
#define STEP_DURATION    1

#define SAMPLE_PERIOD_MS (1000 / FREQ_HZ)                      // ~55ms por amostra
#define WINDOW_SIZE      (FREQ_HZ * WINDOW_DURATION)           // 36 amostras
#define STEP_SIZE        (FREQ_HZ * STEP_DURATION)             // 18 amostras

static const char *TAG = "mpu6050_rules";

// Estrutura para unificar os dados do sensor
typedef struct {
    float ax, ay, az;
    float gx, gy, gz;
} imu_sample_t;

// Buffers para a janela deslizante
static imu_sample_t window_buffer[WINDOW_SIZE];
static int sample_counter = 0;
extern esp_mqtt_client_handle_t client;

// LIMIARES DE CALIBRAÇÃO (Física da queda)
// Nota: 1.0 representa a gravidade normal da Terra (1g)
#define THRESHOLD_FREE_FALL  0.4f   // Abaixo de 0.4g indica perda de sustentação / queda livre
#define THRESHOLD_IMPACT     2.8f   // Acima de 2.8g indica uma colisão severa (impacto no chão)

// Nova abordagem baseada em regras físicas
// Certifique-se de que a variável global do seu cliente MQTT está acessível aqui.
// Exemplo: extern esp_mqtt_client_handle_t client; 

void processar_janela(imu_sample_t *dados, int tamanho)
{
   bool detectou_queda_livre = false;
    bool detectou_impacto = false;
    float max_svm_encontrado = 0.0f;
    float min_svm_encontrado = 1.0f;

    for (int i = 0; i < tamanho; i++) {
        float ax_g = dados[i].ax / 9.80665f;
        float ay_g = dados[i].ay / 9.80665f;
        float az_g = dados[i].az / 9.80665f;

        float svm = sqrtf((ax_g * ax_g) + (ay_g * ay_g) + (az_g * az_g));

        if (svm > max_svm_encontrado) max_svm_encontrado = svm;
        if (svm < min_svm_encontrado) min_svm_encontrado = svm;

        if (svm < THRESHOLD_FREE_FALL) {
            detectou_queda_livre = true;
        }
        if (svm > THRESHOLD_IMPACT) {
            detectou_impacto = true;
        }
    }

    ESP_LOGI(TAG, "Janela analisada -> Pico Max: %.2fg | Pico Min: %.2fg", max_svm_encontrado, min_svm_encontrado);

    // VEREDITO DA QUEDA
    
    if (detectou_queda_livre && detectou_impacto) {
        ESP_LOGW(TAG, "🚨 [ALERTA] QUEDA DETETADA POR REGRAS FÍSICAS!");

        // 1. Acender o LED integrado (Nível lógico Alto/1)
        gpio_set_level(BLINK_GPIO, 1);

        // 2. Notificar o aplicativo Android via Bluetooth Low Energy
        // Passamos o valor '1' que o seu aplicativo já está programado para receber
        ble_notificar_queda(); // Ajuste os parâmetros conforme a assinatura da sua função BLE

        // 3. Publicar no Broker MQTT
        // Nota: O payload do MQTT precisa de ser uma string ou array de caracteres, por isso usamos "9"
        if (client != NULL) {
            int msg_id = esp_mqtt_client_publish(client, "status", "9", 0, 1, 0);
            ESP_LOGI(TAG, "MQTT publicado com sucesso, msg_id=%d", msg_id);
        } else {
            ESP_LOGE(TAG, "Erro: Cliente MQTT não inicializado.");
        }
        
    } else {
        ESP_LOGI(TAG, "✅ Movimento normal seguro.");
        
        // Opcional: Apagar o LED se o movimento voltar ao normal e não for uma queda
        gpio_set_level(BLINK_GPIO, 0);
    }
}

void mpu6050task(void *param) 
{
   mpu6050_dev_t dev = { 0 };

    ESP_ERROR_CHECK(mpu6050_init_desc(&dev, REAL_MPU_ADDR, 0, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO));
    dev.i2c_dev.cfg.sda_pullup_en = 1;
    dev.i2c_dev.cfg.scl_pullup_en = 1;

    while (i2c_dev_probe(&dev.i2c_dev, I2C_DEV_WRITE) != ESP_OK) {
        ESP_LOGE(TAG, "A aguardar sensor...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_ERROR_CHECK(mpu6050_init(&dev));
    ESP_LOGI(TAG, "MPU6050 Inicializado a %d Hz (Modo Regras).", FREQ_HZ);

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(SAMPLE_PERIOD_MS);

    while (1)
    {
       mpu6050_acceleration_t accel = { 0 };
        mpu6050_rotation_t rotation = { 0 };

        if (mpu6050_get_motion(&dev, &accel, &rotation) == ESP_OK)
        {
            window_buffer[sample_counter].ax = accel.x;
            window_buffer[sample_counter].ay = accel.y;
            window_buffer[sample_counter].az = accel.z;
            window_buffer[sample_counter].gx = rotation.x;
            window_buffer[sample_counter].gy = rotation.y;
            window_buffer[sample_counter].gz = rotation.z;

            sample_counter++;

            if (sample_counter >= WINDOW_SIZE)
            {
                processar_janela(window_buffer, WINDOW_SIZE);

                int amostras_restantes = WINDOW_SIZE - STEP_SIZE; 
                memmove(&window_buffer[0], &window_buffer[STEP_SIZE], amostras_restantes * sizeof(imu_sample_t));
                
                sample_counter = amostras_restantes;
            }
        }
        else
        {
            ESP_LOGE(TAG, "Falha de leitura I2C");
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}