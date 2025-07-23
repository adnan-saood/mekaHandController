#define NUM_MOTORS 5


// USB Vendor ID and Product ID (replace with your own)
#define USB_VID 0x303A // Espressif VID, consider getting your own for production
#define USB_PID 0x4001 // Example PID

// USB String Descriptors
// This array defines the strings that the host will query for
// Index 0 is language ID, 1 is manufacturer, 2 is product, 3 is serial
const char* config_usb_string_descriptors[] = {
    (char[]){0x09, 0x04}, // Language: English (0x0409)
    "YourCompany",       // Manufacturer
    "ESP32-S3 HID Device", // Product
    "1234567890ABCDEF"   // Serial number (can be dynamically generated)
};