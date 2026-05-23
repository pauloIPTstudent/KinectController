#ifndef MPU6050_H
#define MPU6050_H
#define I2C_MASTER_SCL_IO           22      // O pino SCL que vimos antes
#define I2C_MASTER_SDA_IO           21      // O pino SDA que vimos antes
#define I2C_MASTER_NUM              I2C_NUM_0
void mpu6050task(void *param);
#endif