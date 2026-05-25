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
// Altere os Limiares de Calibração no topo do arquivo para valores relativos ao seu log:
#define THRESHOLD_IMPACT     1.60f   // Ajustado: Se parado dá 0.10, um impacto real passará de 0.55
#define THRESHOLD_FREE_FALL  0.03f   // Ajustado: Perda quase total de sinal em microgravidade

// Nova abordagem baseada em regras físicas
// Certifique-se de que a variável global do seu cliente MQTT está acessível aqui.
// Exemplo: extern esp_mqtt_client_handle_t client; 

void processar_janela(imu_sample_t *dados, int tamanho)
{
    bool detectou_impacto = false;
    float max_svm_encontrado = 0.0f;
    float min_svm_encontrado = 99.0f;

    for (int i = 0; i < tamanho; i++) {
        float ax = dados[i].ax;
        float ay = dados[i].ay;
        float az = dados[i].az;

        // Calcular a magnitude vetorial (SVM) direta
        float svm = sqrtf((ax * ax) + (ay * ay) + (az * az));

        if (svm > max_svm_encontrado) max_svm_encontrado = svm;
        if (svm < min_svm_encontrado) min_svm_encontrado = svm;

        // Regra Única: Ultrapassou o limite do pico de impacto estabelecido
        if (svm > THRESHOLD_IMPACT) {
            detectou_impacto = true;
        }
    }

    ESP_LOGI(TAG, "Janela analisada -> Pico Max: %.2fg | Pico Min: %.2fg", max_svm_encontrado, min_svm_encontrado);

    // VEREDITO DA QUEDA (Regra direta focada no impacto)
    if (detectou_impacto) {
        ESP_LOGW(TAG, "🚨 [ALERTA] IMPACTO FORTE / QUEDA DETETADA!");

        // 1. Liga o pino do LED externo
        gpio_set_level(BLINK_GPIO, 1);

        // 2. Dispara o Bluetooth Low Energy para a aplicação Android
        ble_notificar_queda(); 

        // 3. Publica a string "9" no Broker MQTT
        if (client != NULL) {
            int msg_id = esp_mqtt_client_publish(client, "status", "9", 0, 1, 0);
            ESP_LOGI(TAG, "MQTT publicado com sucesso, msg_id=%d", msg_id);
        } else {
            ESP_LOGE(TAG, "Erro: Cliente MQTT não inicializado.");
        }
    } else {
        ESP_LOGI(TAG, "✅ Movimento normal seguro.");
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