#pragma once

/*
+-----------+-----+-----------+-----+-----------+-----+
| J0_PWM    |  2  | J0_DIR    |  3  | J1_H      |  4  |
| J1_L      |  5  | J2_H      |  6  | J2_L      |  7  |
| J3_H      |  8  | J3_L      |  9  | J4_H      | 14  |
| J4_L      | 15  | ENC0      | 17  | ENC1      | 18  |
| ENC2      | 21  | SPI_CS    | 10  | SPI_MOSI  | 11  |
| SPI_MISO  | 12  | SPI_CLK   | 13  | IMU_INT   | 16  |
| I2C_SDA   | 35  | I2C_SCL   | 36  | ENC3      | 38  |
| ENC4      | 39  | D1        | 40  | D2        | 41  |
| D3        | 42  |           |     |           |     |
+-----------+-----+-----------+-----+-----------+-----+

*/

// Motor 0 (Joint 0) Pins
#define MOTOR_J0_PWM    GPIO_NUM_2    // PWM output for Joint 0
#define MOTOR_J0_DIR    GPIO_NUM_3    // Direction control for Joint 0

// Motor 1 (Joint 1) Pins
#define MOTOR_J1_PWM_H  GPIO_NUM_4    // PWM high for Joint 1
#define MOTOR_J1_PWM_L  GPIO_NUM_5    // PWM low for Joint 1

// Motor 2 (Joint 2) Pins
#define MOTOR_J2_PWM_H  GPIO_NUM_6    // PWM high for Joint 2
#define MOTOR_J2_PWM_L  GPIO_NUM_7    // PWM low for Joint 2

// Motor 3 (Joint 3) Pins
#define MOTOR_J3_PWM_H  GPIO_NUM_8    // PWM high for Joint 3
#define MOTOR_J3_PWM_L  GPIO_NUM_9    // PWM low for Joint 3

// Motor 4 (Joint 4) Pins
#define MOTOR_J4_PWM_H  GPIO_NUM_14   // PWM high for Joint 4
#define MOTOR_J4_PWM_L  GPIO_NUM_15   // PWM low for Joint 4

// Motor Encoder Pins
#define MOTOR_ENC_0     GPIO_NUM_17   // Encoder for Joint 0
#define MOTOR_ENC_1     GPIO_NUM_18   // Encoder for Joint 1
#define MOTOR_ENC_2     GPIO_NUM_21   // Encoder for Joint 2
#define MOTOR_ENC_3     GPIO_NUM_38   // Encoder for Joint 3
#define MOTOR_ENC_4     GPIO_NUM_39   // Encoder for Joint 4

// LED Pins
#define LED_D1          GPIO_NUM_40   // LED 1
#define LED_D2          GPIO_NUM_41   // LED 2
#define LED_D3          GPIO_NUM_42   // LED 3

// SPI Pins
#define SPI_CS          GPIO_NUM_10   // SPI Chip Select
#define SPI_MOSI        GPIO_NUM_11   // SPI Master Out Slave In
#define SPI_MISO        GPIO_NUM_12   // SPI Master In Slave Out
#define SPI_CLK         GPIO_NUM_13   // SPI Clock

// IMU Interrupt Pin
#define IMU_INT         GPIO_NUM_16   // IMU Interrupt
#define IMU_ENABLE      GPIO_NUM_37   // IMU Enable

// I2C Pins
#define I2C_SDA         GPIO_NUM_35   // I2C Data
#define I2C_SCL         GPIO_NUM_36   // I2C Clock