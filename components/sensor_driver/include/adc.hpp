#pragma once
#include "sensors.hpp"
extern "C"
{
#include <driver/spi_master.h>
#include <esp_log.h>
#include <esp_err.h>
}

#include "pin_config.h"

#define REFIN_VOLTAGE 3.3f

class ADC
{
private:
    spi_device_handle_t spi;

    esp_err_t init_spi()
    {
        esp_err_t ret;
        spi_bus_config_t buscfg = {
            .mosi_io_num = SPI_MOSI,
            .miso_io_num = SPI_MISO,
            .sclk_io_num = SPI_CLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 32};
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
            .queue_size = 7};

        ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
        if (ret != ESP_OK)
            return ret;
        ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
        return ret;
    }

public:
    ADC()
    {
        esp_err_t ret = init_spi();
        if (ret != ESP_OK)
        {
            ESP_LOGE("ADC", "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        }
        else
        {
            ESP_LOGI("ADC", "SPI bus initialized successfully.");
        }
    }

    ADCData read()
    {
        ADCData data;

        // --- Step 1: program the control register ---
        // Configure for:
        //  - WRITE = 1
        //  - SEQ = 1, SHADOW = 1 → continuous sequence from channel 0 to ADDx
        //  - ADD3..0 = 1111 (channel 15, so it cycles 0–15)
        //  - PM1 = PM0 = 1 (normal mode)
        //  - RANGE = 1 (0..REFIN), CODING = 1 (straight binary)
        uint16_t control_word =
            (1 << 11) |            // WRITE = 1
            (1 << 10) | (1 << 3) | // SEQ=1, SHADOW=1
            (0xF << 6) |           // ADD3..0 = 1111 (up to ch15)
            (1 << 5) | (1 << 4) |  // PM1=1, PM0=1 (normal mode)
            (0 << 2) |             // WEAK/TRI = 0
            (1 << 1) | (1 << 0);   // RANGE=1, CODING=1

        uint16_t rx_data = 0;
        spi_transaction_t t = {
            .flags = 0,
            .cmd = 0,
            .addr = 0,
            .length = 16,
            .rxlength = 16,
            .tx_buffer = &control_word,
            .rx_buffer = &rx_data};

        // First transaction: sends control word, returns dummy data
        esp_err_t ret = spi_device_transmit(spi, &t);
        if (ret != ESP_OK)
        {
            ESP_LOGE("ADC", "SPI control word failed: %s", esp_err_to_name(ret));
            return data;
        }

        for (size_t n = 0; n < ADC_CHANNELS; ++n)
        {
            rx_data = 0;
            t.tx_buffer = nullptr; // just clock data out
            t.rx_buffer = &rx_data;

            ret = spi_device_transmit(spi, &t);
            if (ret != ESP_OK)
            {
                ESP_LOGE("ADC", "SPI transaction failed: %s", esp_err_to_name(ret));
                continue;
            }

            // Extract channel number (top 4 bits)
            uint16_t channel_number = (rx_data >> 12) & 0x0F;
            uint16_t adc_raw = rx_data & 0x0FFF;

            if (channel_number < ADC_CHANNELS)
            {
                data.values[channel_number] = static_cast<float>(adc_raw) / 4096.0f;
            }
            else
            {
                ESP_LOGW("ADC", "Invalid channel tag %u (raw=0x%04X)",
                         channel_number, rx_data);
            }
        }

        return data;
    }

    void read_debug(uint8_t channel)
    {
        if (channel > 15)
        {
            ESP_LOGE("ADC", "Invalid channel %u (must be 0–15)", channel);
            return;
        }

        // --- Step 1: build control word for fixed channel ---
        // WRITE=1, SEQ=0, ADD=channel, PM1=PM0=1 (normal mode),
        // RANGE=1 (0..REFIN), CODING=1 (straight binary).
        uint16_t control_word =
            (1 << 11) |              // WRITE
            (0 << 10) |              // SEQ=0
            ((channel & 0xF) << 6) | // ADD3..0 = channel
            (1 << 5) | (1 << 4) |    // PM1=1, PM0=1 (normal mode)
            (0 << 3) | (0 << 2) |    // SHADOW=0, WEAK/TRI=0
            (1 << 1) | (1 << 0);     // RANGE=1, CODING=1

        uint16_t rx_data = 0;
        spi_transaction_t t = {
            .flags = 0,
            .cmd = 0,
            .addr = 0,
            .length = 16,
            .rxlength = 16,
            .tx_buffer = &control_word,
            .rx_buffer = &rx_data};

        // --- Step 2: send control word, discard returned data (dummy) ---
        if (spi_device_transmit(spi, &t) != ESP_OK)
        {
            ESP_LOGE("ADC", "SPI transmit failed during control word");
            return;
        }

        // --- Step 3: read a few frames ---
        for (int n = 0; n < 8; ++n)
        {
            rx_data = 0;
            t.tx_buffer = nullptr; // now just clock data out
            t.rx_buffer = &rx_data;

            if (spi_device_transmit(spi, &t) != ESP_OK)
            {
                ESP_LOGE("ADC", "SPI read failed at sample %d", n);
                continue;
            }

            uint16_t chan = (rx_data >> 12) & 0x0F;
            uint16_t adc_raw = rx_data & 0x0FFF;
            float norm = static_cast<float>(adc_raw) / 4096.0f;

            ESP_LOGI("ADC", "Frame %d: raw=0x%04X | chan=%u | value=%u | norm=%.4f",
                     n, rx_data, chan, adc_raw, norm);
        }
    }
};