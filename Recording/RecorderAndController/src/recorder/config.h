#pragma once

#include <freertos/FreeRTOS.h>
#include <driver/i2s.h>

// sample rate for the system
#define SAMPLE_RATE 16000

// from Marvin Detector DON'T CHANGE!
// Microphone settings
// Hardware left means software right...
#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_RIGHT
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_32
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_25
#define I2S_MIC_SERIAL_DATA GPIO_NUM_33

// i2s config for reading from of I2S
extern i2s_config_t i2s_mic_Config;
// i2s microphone pins
extern i2s_pin_config_t i2s_mic_pins;

// LED
#define LED_PIN GPIO_NUM_19

// SDCard
#define SD_DATA0 GPIO_NUM_2
#define SD_DATA1 GPIO_NUM_4
#define SD_DATA4 GPIO_NUM_12
#define SD_DATA3 GPIO_NUM_13
#define SD_CMD GPIO_NUM_15
