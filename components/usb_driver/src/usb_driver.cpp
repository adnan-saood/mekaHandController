// usb_driver.cpp

#include "usb_driver.hpp"

extern "C"
{
#include "driver/gpio.h"
#include "class/hid/hid_device.h" // For TinyUSB HID specific functions
}
#include <numeric>
#include <iterator>


#define APP_BUTTON GPIO_NUM_0 // Still keeping the button for now, but its role might change
#define TAG "UsbHidDevice"    // Log tag

// --- HID Report Descriptor for our custom 1-byte input/output ---
// Output Report (PC to MCU) - Report ID 1
//   Data: 1 byte (0-255)
// Input Report (MCU to PC) - Report ID 2
//   Data: 1 byte (0-255)
static const uint8_t hid_report_desc[] = {
    0x06, 0x00, 0xFF, // USAGE_PAGE (Vendor Defined Page 1) - Custom page (0xFF00)
    0x09, 0x01,       // USAGE (Vendor Usage 1) - General device usage

    0xA1, 0x01, // COLLECTION (Application) - Top-level collection for our device

    // --- Output Report (PC to MCU) ---
    // This report is for the PC to send a single 8-bit value to the MCU.
    // The PC will send a 2-byte packet: [Report ID 1, Data Byte]
    0x85, 0x01, //   REPORT_ID (1) - Identifier for this output report
    0x09, 0x02, //   USAGE (Vendor Usage 2) - Identifies the input value field
    0x15, 0x00, //   LOGICAL_MINIMUM (0) - Data range 0
    0x25, 0xFF, //   LOGICAL_MAXIMUM (255) - Data range 255
    0x75, 0x08, //   REPORT_SIZE (8) - Each data item is 8 bits (1 byte)
    0x95, 0x0A, //   REPORT_COUNT (10) - There is 10 data items [5 poses and 5 stiffness values]
    0x91, 0x02, //   OUTPUT (Data,Var,Abs)

    // --- Input Report (MCU to PC) ---
    // This report is for the MCU to send a single 8-bit value (incremented) to the PC.
    // The MCU will send a 2-byte packet: [Report ID 2, Data Byte]
    0x85, 0x02, //   REPORT_ID (2) - Identifier for this input report
    0x09, 0x03, //   USAGE (Vendor Usage 3) - Identifies the output value field
    0x15, 0x00, //   LOGICAL_MINIMUM (0)
    0x25, 0xFF, //   LOGICAL_MAXIMUM (255)
    0x75, 0x08, //   REPORT_SIZE (8) - Each data item is 8 bits
    0x95, 0x25, //   REPORT_COUNT (37) - There is 1 data item [5 Poses and 5 Velocities and 5 Forces and 4 quaternion values and 5 MA3 Encoder values and 13 ADC values]
    0x81, 0x02, //   INPUT (Data,Var,Abs)

    0xC0 // END_COLLECTION
};

// --- USB Configuration Descriptor ---
// For a HID device, this defines the interface and its associated endpoint.
// TUD_HID_DESCRIPTOR(_itfnum, _stridx, _boot_protocol, _report_desc_len, _epin, _epsize, _ep_interval)
// The _epin should be an IN endpoint address (e.g., 0x81 for EP1 IN).
// _epsize is the max packet size for the endpoint, often CFG_TUD_HID_EP_BUFSIZE (default 64)
static const uint8_t hid_cfg_desc[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(0, 4, false, sizeof(hid_report_desc), 0x81, CFG_TUD_HID_EP_BUFSIZE, 10) // Fixed arguments here
};

// --- USB String Descriptors ---
static const char *hid_str_desc[] = {
    (const char[]){0x09, 0x04}, // Language: English (0x0409)
    "ENSTA",                    // iManufacturer
    "ESP32 HID Interface",      // iProduct
    "20250725",                 // iSerialNumber
    "Incrementer Interface",    // iInterface (index 4 in hid_cfg_desc)
};

// --- Global UsbHidDevice instance pointer ---
// This is the bridge between the C-style TinyUSB callbacks and your C++ class.
UsbHidDevice *g_usb_hid_device_instance = nullptr;

// --- Global TinyUSB Callbacks (extern "C" to be compatible with TinyUSB C API) ---

// Invoked when received GET HID REPORT DESCRIPTOR request
// Application returns pointer to descriptor
extern "C" const uint8_t *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance; // Unused parameter
    return hid_report_desc;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer with the requested report
extern "C" uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)instance;
    (void)report_type;
    (void)reqlen;
    ESP_LOGI(TAG, "GET_REPORT callback: Instance %d, Report ID %d, Type %d, ReqLen %d", instance, report_id, report_type, reqlen);

    if (report_id == 0x02 && reqlen >= 1 && g_usb_hid_device_instance != nullptr)
    {
        if (xSemaphoreTake(g_usb_hid_device_instance->getMutex(), pdMS_TO_TICKS(10)))
        {
            buffer[0] = g_usb_hid_device_instance->getValueToSendBack();
            xSemaphoreGive(g_usb_hid_device_instance->getMutex());
            return 1; // Indicate 1 byte of data provided
        }
    }
    return 0; // Indicate no data or not handled
}

// Invoked when received SET_REPORT control request
// Application must consume the control data and return the length of data consumed
extern "C" void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, const uint8_t *buffer, uint16_t bufsize)
{
    (void)instance;
    (void)report_type;
    ESP_LOGI(TAG, "SET_REPORT callback: Instance %d, Report ID %d, Type %d, BufSize %d", instance, report_id, report_type, bufsize);

    if (g_usb_hid_device_instance != nullptr)
    {
        if (xSemaphoreTake(g_usb_hid_device_instance->getMutex(), pdMS_TO_TICKS(10)))
        {
            g_usb_hid_device_instance->handleSetReport(report_id, buffer, bufsize);
            xSemaphoreGive(g_usb_hid_device_instance->getMutex());
        }
        else
        {
            ESP_LOGW(TAG, "SET_REPORT: Could not acquire mutex.");
        }
    }
    else
    {
        ESP_LOGW(TAG, "SET_REPORT: UsbHidDevice instance not set!");
    }
}

// --- UsbHidDevice Class Implementation ---

UsbHidDevice::UsbHidDevice() : received_value_(0),
                               value_to_send_back_(0),
                               new_value_available_(false)
{
    mutex_ = xSemaphoreCreateMutex();
    if (mutex_ == NULL)
    {
        ESP_LOGE(TAG, "Failed to create mutex");
        gpio_set_level(GPIO_NUM_13, 1);
        vTaskDelay(pdMS_TO_TICKS(200)); // Keep LED on for a short duration
        gpio_set_level(GPIO_NUM_13, 0);
        vTaskDelay(pdMS_TO_TICKS(100)); // Keep LED on for a short duration
    }
}

void UsbHidDevice::init()
{
    // 1. Initialize GPIO
    const gpio_config_t btn_cfg = {
        .pin_bit_mask = BIT64(APP_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&btn_cfg);

    // 1. Initialize GPIO
    const gpio_config_t led_cfg = {
        .pin_bit_mask = BIT64(GPIO_NUM_13),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&led_cfg);
    ESP_LOGI(TAG, "LED initialized.");

    static const tusb_desc_device_t my_device_desc = {
        .bLength = sizeof(tusb_desc_device_t),
        .bDescriptorType = TUSB_DESC_DEVICE,
        .bcdUSB = 0x0200,
        .bDeviceClass = 0,
        .bDeviceSubClass = 0,
        .bDeviceProtocol = 0,
        .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
        .idVendor = 0xCafe,
        .idProduct = 0x4000,
        .bcdDevice = 0x0100,
        .iManufacturer = 0x01,
        .iProduct = 0x02,
        .iSerialNumber = 0x03,
        .bNumConfigurations = 0x01};

    // 2. Initialize TinyUSB driver
    const tinyusb_config_t tusb_cfg = {
        // EXACT ORDER matching your provided example and including string_descriptor_count
        .device_descriptor = &my_device_desc, // TinyUSB will use default device descriptor if NULL
        .string_descriptor = hid_str_desc,
        .string_descriptor_count = sizeof(hid_str_desc) / sizeof(hid_str_desc[0]),
        .external_phy = false,
#if TUD_OPT_HIGH_SPEED
        .fs_configuration_descriptor = hid_cfg_desc,
        .hs_configuration_descriptor = hid_cfg_desc,
        .qualifier_descriptor = NULL, // Re-added this as per your example
#else
        .configuration_descriptor = hid_cfg_desc,
#endif // TUD_OPT_HIGH_SPEED
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "TinyUSB driver installed.");
}

// This method is called by the extern "C" tud_hid_set_report_cb
void UsbHidDevice::handleSetReport(uint8_t report_id, const uint8_t *buffer, uint16_t bufsize)
{
    // light up led on GPIO_13 when a report is received
    gpio_set_level(GPIO_NUM_13, 1);
    if (report_id == 0x01 && bufsize >= 10) {
        for (int i = 0; i < 10; ++i) {
            received_value_[i] = buffer[i];
        }
        new_value_available_ = true;
    }
}

// Method to send the incremented value back to the PC
void UsbHidDevice::sendIncrementedValue()
{
    if (!tud_hid_ready())
        return;

    // Prepare a buffer with numbers 1 to 37
    uint8_t payload_data[37];
    for (uint8_t i = 0; i < 37; ++i) {
        payload_data[i] = i + 1;
    }

    tud_hid_report(0x02, payload_data, sizeof(payload_data));
    new_value_available_ = false; // Reset the flag after sending
    gpio_set_level(GPIO_NUM_13, 0);
}

// Main task loop
void UsbHidDevice::taskLoop()
{
    while (1)
    {
        if (tud_mounted())
        {
            if (new_value_available_)
            {
                sendIncrementedValue();
            }
        }
    }
    vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to yield to other tasks
}
