#pragma once
#include "sensors.hpp"
extern "C" {
    #include <driver/spi_master.h>
    #include <esp_log.h>
    #include <esp_err.h>
}

#include "pin_config.h"

#define REFIN_VOLTAGE 3.3f

class ADC {
private:
    spi_device_handle_t spi;

    esp_err_t init_spi() {
        esp_err_t ret;
        spi_bus_config_t buscfg = {
            .mosi_io_num = SPI_MOSI,
            .miso_io_num = SPI_MISO,
            .sclk_io_num = SPI_CLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 32
        };
        spi_device_interface_config_t devcfg = {
            .command_bits = 0,
            .address_bits = 0,
            .dummy_bits = 0,
            .mode = 2, // AD7490 operates in SPI Mode 2 (CPOL=1, CPHA=1)
            .duty_cycle_pos = 128,
            .cs_ena_pretrans = 0,
            .cs_ena_posttrans = 0,
            .clock_speed_hz = 10000000, // 10 MHz SCLK is a safe value.
            .input_delay_ns = 0,
            .spics_io_num = SPI_CS,
            .flags = SPI_DEVICE_NO_DUMMY,
            .queue_size = 7
        };

        ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
        if (ret != ESP_OK) return ret;
        ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
        return ret;
    }

public:
    ADC() {
        esp_err_t ret = init_spi();
        if (ret != ESP_OK) {
            ESP_LOGE("ADC", "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        } else {
            ESP_LOGI("ADC", "SPI bus initialized successfully.");
        }
    }

    ADCData read() {
        ADCData data;

        // Command to start sequencer mode on channel 0
        // WRITE(1), SEQ(1), ADD3-ADD0(0), PM1-PM0(0), SHADOW(0), WEAK/TRI(0), RANGE(1), CODING(1)
        uint16_t control_word_seq_start = (1 << 11) | (1 << 10) | (0 << 6) | (0 << 5) | (0 << 4) | (0 << 3) | (0 << 2) | (1 << 1) | (1 << 0);
        
        spi_transaction_t t = {
            .flags = 0,
            .cmd = 0,
            .addr = 0,
            .length = 16,
            .rxlength = 16,
            .user = (void*)0,
            .tx_buffer = &control_word_seq_start,
            .rx_buffer = nullptr
        };
        
        for (size_t i = 0; i < ADC_CHANNELS; ++i) {
            uint16_t rx_data;
            t.rx_buffer = &rx_data;
            t.tx_buffer = nullptr; // For subsequent reads, no command word is needed.

            esp_err_t ret = spi_device_transmit(spi, &t);
            if (ret != ESP_OK) {
                ESP_LOGE("ADC", "SPI transaction failed for channel %zu: %s", i, esp_err_to_name(ret));
                data.values[i] = 0.0f;
                continue;
            }

            // The datasheet states the data is 4 address bits followed by 12 data bits.
            // The 12-bit ADC value is in the lower 12 bits of the 16-bit response.

            // extract the channel number and confirm it's the expected channel
            uint16_t channel_number = (rx_data >> 12) & 0x0F; // Get the channel number from the upper 4 bits
            if (channel_number != i) {
                ESP_LOGW("ADC", "Unexpected channel number %u for channel %zu", channel_number, i);
                data.values[i] = 0.0f; // Set to 0 if unexpected channel
                continue;
            }

            uint16_t adc_raw_value = (rx_data >> 4);
            
            // Convert the raw 12-bit ADC value to a voltage
            data.values[i] = (static_cast<float>(adc_raw_value) / 4096.0f);
        }
        
        return data;
    }
};