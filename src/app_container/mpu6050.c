#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_log.h>
#include <mpu6050.h>
#include <math.h>
#include "app_container/fall_model_scientific.h"
#include "app_container/mpu6050.h"

#define REAL_MPU_ADDR MPU6050_I2C_ADDRESS_LOW 

// Definições da Janela e Frequência
#define FREQ_HZ          18
#define WINDOW_DURATION  2
#define STEP_DURATION    1

#define SAMPLE_PERIOD_MS (1000 / FREQ_HZ)                      // ~55ms por amostra
#define WINDOW_SIZE      (FREQ_HZ * WINDOW_DURATION)           // 36 amostras
#define STEP_SIZE        (FREQ_HZ * STEP_DURATION)             // 18 amostras

#define ESCALA_IA 10000.0f
static const char *TAG = "mpu6050_window";

// Estrutura para unificar os dados que queres guardar
typedef struct {
    float ax, ay, az;
    float gx, gy, gz;
} imu_sample_t;

// Buffers para a janela deslizante
static imu_sample_t window_buffer[WINDOW_SIZE];
static int sample_counter = 0;

// Função para calcular Média, Max Amplitude, Min Amplitude e Desvio Padrão
void calcular_estatisticas(float *dados_eixo, int tamanho, float *media, float *max_amp, float *min_amp, float *desvio)
{
    float soma = 0.0;
    float max_val = dados_eixo[0];
    float min_val = dados_eixo[0];

    // Primeira passagem: calcular soma, máximo e mínimo
    for (int i = 0; i < tamanho; i++) {
        soma += dados_eixo[i];
        if (dados_eixo[i] > max_val) max_val = dados_eixo[i];
        if (dados_eixo[i] < min_val) min_val = dados_eixo[i];
    }
    *media = soma / tamanho;
    *max_amp = max_val;
    *min_amp = min_val;

    // Segunda passagem: calcular a variância para obter o Desvio Padrão
    float soma_quadrados = 0.0;
    for (int i = 0; i < tamanho; i++) {
        soma_quadrados += powf(dados_eixo[i] - (*media), 2);
    }
    *desvio = sqrtf(soma_quadrados / tamanho);
}

// Nova função de processamento da janela com IA integrada
void processar_janela(imu_sample_t *dados, int tamanho)
{
    // Arrays temporários para separar os eixos e facilitar as contas
    float eixo_x[WINDOW_SIZE];
    float eixo_y[WINDOW_SIZE];
    float eixo_z[WINDOW_SIZE];

    for (int i = 0; i < tamanho; i++) {
        eixo_x[i] = dados[i].ax;
        eixo_y[i] = dados[i].ay;
        eixo_z[i] = dados[i].az;
    }

    // Variáveis para guardar as 12 features extraídas
    float x_mean, x_max, x_min, x_std;
    float y_mean, y_max, y_min, y_std;
    float z_mean, z_max, z_min, z_std;

    // Calcular as estatísticas para cada eixo do acelerómetro do pulso
    calcular_estatisticas(eixo_x, tamanho, &x_mean, &x_max, &x_min, &x_std);
    calcular_estatisticas(eixo_y, tamanho, &y_mean, &y_max, &y_min, &y_std);
    calcular_estatisticas(eixo_z, tamanho, &z_mean, &z_max, &z_min, &z_std);

    // Array de input para o emlearn (DEVE seguir rigorosamente a ordem do Python)
    float features[12] = {
        x_mean, x_max, x_min, x_std,
        y_mean, y_max, y_min, y_std,
        z_mean, z_max, z_min, z_std
    };

    // Executar a inferência da Random Forest no ESP32
    // Nota: 'fall_model_scientific_predict' é o nome padrão gerado com base no nome do ficheiro .h
    // Se o emlearn gerar outro nome interno, verifica no ficheiro .h qual é o nome da função de predict.
    int resultado = fall_model_scientific_predict(features, 12);

    // Apresentar o resultado no terminal
    ESP_LOGW(TAG, "===> INFERÊNCIA DA JANELA REALIZADA <===");
    if (resultado == 1) {
        ESP_LOGE(TAG, "⚠️ ALERTA: QUEDA DETETADA! (Classe 1)");
    } else {
        ESP_LOGI(TAG, "✅ Estado: Normal / Não-Queda (Classe 0)");
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
    ESP_LOGI(TAG, "MPU6050 Inicializado a %d Hz.", FREQ_HZ);

    // Variável para controlar o tempo preciso do loop
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(SAMPLE_PERIOD_MS);

    while (1)
    {
        mpu6050_acceleration_t accel = { 0 };
        mpu6050_rotation_t rotation = { 0 };

        // 1. Leitura do sensor
        if (mpu6050_get_motion(&dev, &accel, &rotation) == ESP_OK)
        {
            // Guardar no buffer na posição atual
            window_buffer[sample_counter].ax = accel.x;
            window_buffer[sample_counter].ay = accel.y;
            window_buffer[sample_counter].az = accel.z;
            window_buffer[sample_counter].gx = rotation.x;
            window_buffer[sample_counter].gy = rotation.y;
            window_buffer[sample_counter].gz = rotation.z;

            sample_counter++;

            // 2. Verificar se a janela de 2 segundos (36 amostras) está cheia
            if (sample_counter >= WINDOW_SIZE)
            {
                // Envia o buffer completo para processamento
                processar_janela(window_buffer, WINDOW_SIZE);

                // 3. Aplicar o Deslocamento (Step) de 1 segundo (18 amostras)
                // Move os últimos 1 segundo de dados para o início do buffer
                int amostras_restantes = WINDOW_SIZE - STEP_SIZE; // 36 - 18 = 18 amostras
                memmove(&window_buffer[0], &window_buffer[STEP_SIZE], amostras_restantes * sizeof(imu_sample_t));
                
                // O contador volta a apontar para o espaço vazio (posição 18)
                sample_counter = amostras_restantes;
            }
        }
        else
        {
            ESP_LOGE(TAG, "Falha de leitura I2C");
        }

        // 4. Temporização de Alta Precisão (Evita drifting/atrasos acumulados)
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}