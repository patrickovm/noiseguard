#include "ds18b20.h"
#include "pico/stdlib.h"

bool ds18b20_init(DS18B20 *dev, PIO pio, uint gpio) {
    uint offset = pio_add_program(pio, &onewire_program);
    dev->initialized = ow_init(&dev->ow, pio, offset, gpio);
    
    if (dev->initialized) {
        // Search for first DS18B20 device
        uint64_t rom_codes[1];
        if (ow_romsearch(&dev->ow, rom_codes, 1, 0xF0) > 0) {
            dev->rom_code = rom_codes[0];
            return true;
        }
    }
    return false;
}

float ds18b20_read_temp(DS18B20 *dev) {
    if (!dev->initialized) {
        return -999.0f;  // Return error value if not initialized
    }

    uint8_t scratchpad[9];
    
    // Reset and select device
    ow_reset(&dev->ow);
    ow_send(&dev->ow, 0x55);  // Match ROM command
    
    // Send ROM code byte by byte (LSB first)
    for (int i = 0; i < 8; i++) {
        ow_send(&dev->ow, (dev->rom_code >> (i * 8)) & 0xFF);
    }
    
    // Start temperature conversion
    ow_send(&dev->ow, 0x44);  // Convert T command
    sleep_ms(750);  // Wait for conversion
    
    // Read scratchpad
    ow_reset(&dev->ow);
    ow_send(&dev->ow, 0x55);  // Match ROM command
    
    // Send ROM code again
    for (int i = 0; i < 8; i++) {
        ow_send(&dev->ow, (dev->rom_code >> (i * 8)) & 0xFF);
    }
    
    ow_send(&dev->ow, 0xBE);  // Read Scratchpad command
    
    // Read all 9 bytes of scratchpad
    for (int i = 0; i < 9; i++) {
        scratchpad[i] = ow_read(&dev->ow);
    }
    
    // Convert temperature
    int16_t temp = (scratchpad[1] << 8) | scratchpad[0];
    float temperature = temp * 0.0625f;  // Each bit = 0.0625°C
    
    return temperature;
}