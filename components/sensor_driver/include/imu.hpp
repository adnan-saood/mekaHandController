#pragma once
#include "sensors.hpp"

extern "C"
{

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "driver/i2c.h"
#include "esp_log.h"
#include "icm20948.h"
#include "icm20948_i2c.h"

#include <math.h>
}

#include "pin_config.h"

class IMU
{
public:
    IMU()
    {
        /* i2c bus configuration */
        ESP_LOGI("IMU", "Initializing I2C for IMU");
        i2c_config_t conf = {};
        conf.mode = I2C_MODE_MASTER;
        conf.sda_io_num = (gpio_num_t)I2C_SDA;
        conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
        conf.scl_io_num = (gpio_num_t)I2C_SCL;
        conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
        conf.master.clk_speed = 400000;
        conf.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;

        /* ICM 20948 configuration */
        icm0948_config_i2c_t icm_config = {
            .i2c_port = I2C_NUM_0,
            .i2c_addr = ICM_20948_I2C_ADDR_AD0};

        conf_ = conf;
        icm_config_ = icm_config;

        
    }

    esp_err_t init()
    {
        ESP_ERROR_CHECK(i2c_param_config(icm_config_.i2c_port, &conf_));
        ESP_ERROR_CHECK(i2c_driver_install(icm_config_.i2c_port, conf_.mode, 0, 0, 0));

        ESP_LOGI("IMU", "I2C initialized for IMU");

        while (icm20948_check_id(&icm) != ICM_20948_STAT_OK)
        {
            ESP_LOGE("IMU", "check id failed");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI("IMU", "check id passed");

        icm20948_init_i2c(&icm, &icm_config_);

        ESP_LOGI("IMU", "ICM20948 initialized");

        /* Here we are doing a SW reset to make sure the device starts in a known state */
        icm20948_sw_reset(&icm);
        vTaskDelay(250 / portTICK_PERIOD_MS);

        icm20948_internal_sensor_id_bm sensors = (icm20948_internal_sensor_id_bm)(ICM_20948_INTERNAL_ACC | ICM_20948_INTERNAL_GYR);
        icm20948_set_sample_mode(&icm, sensors, SAMPLE_MODE_CONTINUOUS);

        icm20948_fss_t myfss;
        myfss.a = GPM_2;   // (icm20948_accel_config_fs_sel_e)
        myfss.g = DPS_250; // (icm20948_gyro_config_1_fs_sel_e)
        icm20948_set_full_scale(&icm, sensors, myfss);

        // Set up DLPF configuration
        icm20948_dlpcfg_t myDLPcfg;
        myDLPcfg.a = ACC_D473BW_N499BW;
        myDLPcfg.g = GYR_D361BW4_N376BW5;
        icm20948_set_dlpf_cfg(&icm, sensors, myDLPcfg);

        ESP_LOGI("IMU", "DLPF configured for IMU");

        // Choose whether or not to use DLPF
        icm20948_enable_dlpf(&icm, ICM_20948_INTERNAL_ACC, false);
        icm20948_enable_dlpf(&icm, ICM_20948_INTERNAL_GYR, false);

        // Now wake the sensor up
        icm20948_sleep(&icm, false);
        icm20948_low_power(&icm, false);

        /* now the fun with DMP starts */
        init_dmp(&icm);

        return ESP_OK;
    }

    IMUData read()
    {
        IMUData ans;
        icm_20948_DMP_data_t data;
		icm20948_status_e status = inv_icm20948_read_dmp_data(&icm, &data);
		/* Was valid data available? */
  		if ((status == ICM_20948_STAT_OK) || (status == ICM_20948_STAT_FIFO_MORE_DATA_AVAIL)) 
		{
			/* We have asked for orientation data so we should receive Quat9 */
			if ((data.header & DMP_header_bitmap_Quat9) > 0) 
			{
				// Q0 value is computed from this equation: Q0^2 + Q1^2 + Q2^2 + Q3^2 = 1.
				// In case of drift, the sum will not add to 1, therefore, quaternion data need to be corrected with right bias values.
				// The quaternion data is scaled by 2^30.
				// Scale to +/- 1
				double q1 = ((double)data.Quat9.Data.Q1) / 1073741824.0; // Convert to double. Divide by 2^30
				double q2 = ((double)data.Quat9.Data.Q2) / 1073741824.0; // Convert to double. Divide by 2^30
				double q3 = ((double)data.Quat9.Data.Q3) / 1073741824.0; // Convert to double. Divide by 2^30
				double q0 = sqrt(1.0 - ((q1 * q1) + (q2 * q2) + (q3 * q3)));
				//ESP_LOGI(TAG, "Q1: %f Q2: %f Q3: %f Accuracy: %d", q1, q2, q3, data.Quat9.Data.Accuracy);
                ans.quaternion = {static_cast<float>(q0),
                                  static_cast<float>(q1),
                                  static_cast<float>(q2), 
                                  static_cast<float>(q3)};
			}

		}
        return ans;
    }

protected:
    i2c_config_t conf_;
    icm0948_config_i2c_t icm_config_;

    icm20948_device_t icm;

    void init_dmp(icm20948_device_t *icm)
    {
        ESP_LOGI("IMU", "Initializing DMP for IMU");
        bool success = true; // Use success to show if the DMP configuration was successful

        // Initialize the DMP with defaults.
        success &= (icm20948_init_dmp_sensor_with_defaults(icm) == ICM_20948_STAT_OK);
        // DMP sensor options are defined in ICM_20948_DMP.h
        //    INV_ICM20948_SENSOR_ACCELEROMETER               (16-bit accel)
        //    INV_ICM20948_SENSOR_GYROSCOPE                   (16-bit gyro + 32-bit calibrated gyro)
        //    INV_ICM20948_SENSOR_RAW_ACCELEROMETER           (16-bit accel)
        //    INV_ICM20948_SENSOR_RAW_GYROSCOPE               (16-bit gyro + 32-bit calibrated gyro)
        //    INV_ICM20948_SENSOR_MAGNETIC_FIELD_UNCALIBRATED (16-bit compass)
        //    INV_ICM20948_SENSOR_GYROSCOPE_UNCALIBRATED      (16-bit gyro)
        //    INV_ICM20948_SENSOR_STEP_DETECTOR               (Pedometer Step Detector)
        //    INV_ICM20948_SENSOR_STEP_COUNTER                (Pedometer Step Detector)
        //    INV_ICM20948_SENSOR_GAME_ROTATION_VECTOR        (32-bit 6-axis quaternion)
        //    INV_ICM20948_SENSOR_ROTATION_VECTOR             (32-bit 9-axis quaternion + heading accuracy)
        //    INV_ICM20948_SENSOR_GEOMAGNETIC_ROTATION_VECTOR (32-bit Geomag RV + heading accuracy)
        //    INV_ICM20948_SENSOR_GEOMAGNETIC_FIELD           (32-bit calibrated compass)
        //    INV_ICM20948_SENSOR_GRAVITY                     (32-bit 6-axis quaternion)
        //    INV_ICM20948_SENSOR_LINEAR_ACCELERATION         (16-bit accel + 32-bit 6-axis quaternion)
        //    INV_ICM20948_SENSOR_ORIENTATION                 (32-bit 9-axis quaternion + heading accuracy)

        // Enable the DMP orientation sensor
        success &= (inv_icm20948_enable_dmp_sensor(icm, INV_ICM20948_SENSOR_ORIENTATION, 1) == ICM_20948_STAT_OK);

        // Enable any additional sensors / features
        // success &= (myICM.enableDMPSensor(INV_ICM20948_SENSOR_RAW_GYROSCOPE) == ICM_20948_STAT_OK);
        // success &= (myICM.enableDMPSensor(INV_ICM20948_SENSOR_RAW_ACCELEROMETER) == ICM_20948_STAT_OK);
        // success &= (myICM.enableDMPSensor(INV_ICM20948_SENSOR_MAGNETIC_FIELD_UNCALIBRATED) == ICM_20948_STAT_OK);

        // Configuring DMP to output data at multiple ODRs:
        // DMP is capable of outputting multiple sensor data at different rates to FIFO.
        // Setting value can be calculated as follows:
        // Value = (DMP running rate / ODR ) - 1
        // E.g. For a 5Hz ODR rate when DMP is running at 55Hz, value = (55/5) - 1 = 10.
        success &= (inv_icm20948_set_dmp_sensor_period(icm, DMP_ODR_Reg_Quat9, 0) == ICM_20948_STAT_OK); // Set to the maximum
        // success &= (myICM.setDMPODRrate(DMP_ODR_Reg_Accel, 0) == ICM_20948_STAT_OK); // Set to the maximum
        // success &= (myICM.setDMPODRrate(DMP_ODR_Reg_Gyro, 0) == ICM_20948_STAT_OK); // Set to the maximum
        // success &= (myICM.setDMPODRrate(DMP_ODR_Reg_Gyro_Calibr, 0) == ICM_20948_STAT_OK); // Set to the maximum
        // success &= (myICM.setDMPODRrate(DMP_ODR_Reg_Cpass, 0) == ICM_20948_STAT_OK); // Set to the maximum
        // success &= (myICM.setDMPODRrate(DMP_ODR_Reg_Cpass_Calibr, 0) == ICM_20948_STAT_OK); // Set to the maximum
        //  Enable the FIFO
        success &= (icm20948_enable_fifo(icm, true) == ICM_20948_STAT_OK);
        // Enable the DMP
        success &= (icm20948_enable_dmp(icm, 1) == ICM_20948_STAT_OK);
        // Reset DMP
        success &= (icm20948_reset_dmp(icm) == ICM_20948_STAT_OK);
        // Reset FIFO
        success &= (icm20948_reset_fifo(icm) == ICM_20948_STAT_OK);

        // Check success
        if (success)
        {
            ESP_LOGI("IMU", "DMP enabled!");
        }
        else
        {
            ESP_LOGE("IMU", "Enable DMP failed!");
            while (1)
                ; // Do nothing more
        }
    }
};