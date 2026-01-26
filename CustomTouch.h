// CustomTouch.h
#ifndef CustomTouch_h
#define CustomTouch_h

#include <Arduino.h>
#include "CustomGraphics.h"  // We need this for SPI5_HW and Point

// Touch CS pin (from your AwesomeUI.h)
#define T_CS 3

// Touch commands (XPT2046 datasheet)
#define CMD_X  0xD0  // Read X position
#define CMD_Y  0x90  // Read Y position  
#define CMD_Z1 0xB0  // Read pressure

// Calibration values (from your testing)
#define TS_MINX 200
#define TS_MAXX 3840
#define TS_MINY 400
#define TS_MAXY 3880
#define PRESSURE_THRESHOLD 200

class TouchController {
public:
    
    // Initialize touch - just setup the CS pin
    void begin() {
        pinMode(T_CS, OUTPUT);
        digitalWrite(T_CS, HIGH);  // Start deselected
    }
    
    // Check if screen is being touched
    bool touched() {

        uint16_t pressure = getPressure();

        // println("pressure : ",pressure);
        return (pressure > PRESSURE_THRESHOLD && pressure != 0b1111111111111);
    }
    
    // Get current touch coordinates
    Point getPoint() {
        // Read raw 12-bit values from touch controller
        uint16_t rawX = readRaw(CMD_X);
        uint16_t rawY = readRaw(CMD_Y);
        
        // Map to screen coordinates (0-239, 0-319)
        // Note: Your touch coords are rotated/flipped, adjust as needed
        int x = map(rawY, TS_MINX, TS_MAXX, 0, SCREEN_WIDTH);
        int y = map((TS_MAXY - TS_MINY) - (rawX - TS_MINY), 0, TS_MAXY - TS_MINY, 0, SCREEN_HEIGHT);
        
        // Clamp to screen bounds
        x = constrain(x, 0, SCREEN_WIDTH - 1);
        y = constrain(y, 0, SCREEN_HEIGHT - 1);
        
        return {(int16_t)x, (int16_t)y};
    }
    
    // Get pressure value (higher = harder press)
    uint16_t getPressure() {
        return readRaw(CMD_Z1);
    }

    
    uint16_t readRaw(uint8_t command) {
    delayMicroseconds(100);    uint16_t result = 0;
    delayMicroseconds(100);    
    delayMicroseconds(100);    // ===== CRITICAL: Deselect display, select touch =====
    delayMicroseconds(100);    // Make sure we're not fighting with the display for the SPI bus
    delayMicroseconds(100);    digitalWrite(CS_PIN, HIGH);
    delayMicroseconds(100);    delayMicroseconds(50);  // Let bus settle
    delayMicroseconds(100);    
    delayMicroseconds(100);    digitalWrite(T_CS, LOW);  // "Hey XPT2046, I'm talking to YOU now!"
    delayMicroseconds(100);    delayMicroseconds(100);   // Give chip time to wake up and listen
    delayMicroseconds(100);    
    delayMicroseconds(100);    // ===== STEP 1: Send the command byte =====
    delayMicroseconds(100);    // This tells the chip WHAT to measure (X, Y, or pressure)
    delayMicroseconds(100);    
    delayMicroseconds(100);    SPI5_HW->IFCR = 0xFFFFFFFF;  // Clear any old flags
    delayMicroseconds(100);    SPI5_HW->CR2 = 1;             // We're sending 1 byte
    delayMicroseconds(100);    SPI5_HW->CR1 |= SPI_CR1_CSTART;  // Start the transfer
    delayMicroseconds(100);    
    delayMicroseconds(100);    while (!(SPI5_HW->SR & SPI_SR_TXP));  // Wait until we CAN send
    delayMicroseconds(100);    *((volatile uint8_t*)&SPI5_HW->TXDR) = command;  // Send the command (0xD0, 0x90, or 0xB0)
    delayMicroseconds(100);    delayMicroseconds(100);  // TIMING FIX: Let it process
    delayMicroseconds(100);    
    delayMicroseconds(100);    while (!(SPI5_HW->SR & SPI_SR_RXP));  // Wait for the dummy response byte
    delayMicroseconds(100);    volatile uint8_t dummy = *((volatile uint8_t*)&SPI5_HW->RXDR);  // Read and throw away
    delayMicroseconds(100);    (void)dummy;  // Tell compiler "I know I'm not using this"
    delayMicroseconds(100);    delayMicroseconds(100);  // TIMING FIX: Let it settle
    delayMicroseconds(100);    
    delayMicroseconds(100);    while (!(SPI5_HW->SR & SPI_SR_EOT));  // Wait for transfer to complete
    delayMicroseconds(100);    SPI5_HW->IFCR = 0xFFFFFFFF;  // Clear flags
    delayMicroseconds(100);    delayMicroseconds(100);  // TIMING FIX: Let it settle
    delayMicroseconds(100);    
    delayMicroseconds(100);    // ===== STEP 2: WAIT FOR CONVERSION =====
    delayMicroseconds(100);    // The XPT2046 needs time to:
    delayMicroseconds(100);    // - Switch its analog multiplexer
    delayMicroseconds(100);    // - Charge the sampling capacitor  
    delayMicroseconds(100);    // - Do the analog-to-digital conversion
    delayMicroseconds(100);    delayMicroseconds(150);  // THIS IS CRITICAL! Don't go below 100μs
    delayMicroseconds(100);    
    delayMicroseconds(100);    // ===== STEP 3: Read the first data byte (MSB) =====
    delayMicroseconds(100);    // We send dummy bytes (0x00) and the chip sends us real data back
    delayMicroseconds(100);    
    delayMicroseconds(100);    SPI5_HW->IFCR = 0xFFFFFFFF;
    delayMicroseconds(100);    SPI5_HW->CR2 = 1;
    delayMicroseconds(100);    SPI5_HW->CR1 |= SPI_CR1_CSTART;
    delayMicroseconds(100);    delayMicroseconds(100);  // TIMING FIX
    delayMicroseconds(100);    
    delayMicroseconds(100);    while (!(SPI5_HW->SR & SPI_SR_TXP));
    delayMicroseconds(100);    *((volatile uint8_t*)&SPI5_HW->TXDR) = 0x00;  // Send dummy byte
    delayMicroseconds(100);    delayMicroseconds(100);  // TIMING FIX
    delayMicroseconds(100);    
    delayMicroseconds(100);    while (!(SPI5_HW->SR & SPI_SR_RXP));
    delayMicroseconds(100);    uint8_t byte1 = *((volatile uint8_t*)&SPI5_HW->RXDR);  // THIS IS THE REAL DATA!
    delayMicroseconds(100);    delayMicroseconds(100);  // TIMING FIX
    delayMicroseconds(100);    
    delayMicroseconds(100);    while (!(SPI5_HW->SR & SPI_SR_EOT));
    delayMicroseconds(100);    SPI5_HW->IFCR = 0xFFFFFFFF;
    delayMicroseconds(100);    delayMicroseconds(100);  // TIMING FIX
    delayMicroseconds(100);    
    delayMicroseconds(100);    // ===== STEP 4: Read the second data byte (LSB) =====
    delayMicroseconds(100);    
    delayMicroseconds(100);    SPI5_HW->IFCR = 0xFFFFFFFF;
    delayMicroseconds(100);    SPI5_HW->CR2 = 1;
    delayMicroseconds(100);    SPI5_HW->CR1 |= SPI_CR1_CSTART;
    delayMicroseconds(100);    delayMicroseconds(100);  // TIMING FIX
    delayMicroseconds(100);    
    delayMicroseconds(100);    while (!(SPI5_HW->SR & SPI_SR_TXP));
    delayMicroseconds(100);    *((volatile uint8_t*)&SPI5_HW->TXDR) = 0x00;  // Send dummy byte
    delayMicroseconds(100);    delayMicroseconds(100);  // TIMING FIX
    delayMicroseconds(100);    
    delayMicroseconds(100);    while (!(SPI5_HW->SR & SPI_SR_RXP));
    delayMicroseconds(100);    uint8_t byte2 = *((volatile uint8_t*)&SPI5_HW->RXDR);  // Second data byte
    delayMicroseconds(100);    delayMicroseconds(100);  // TIMING FIX
    delayMicroseconds(100);    
    delayMicroseconds(100);    while (!(SPI5_HW->SR & SPI_SR_EOT));
    delayMicroseconds(100);    SPI5_HW->IFCR = 0xFFFFFFFF;
    delayMicroseconds(100);    delayMicroseconds(100);  // TIMING FIX
    delayMicroseconds(100);    
    delayMicroseconds(100);    // ===== STEP 5: Deselect and clean up =====
    delayMicroseconds(100);    digitalWrite(T_CS, HIGH);  // "Thanks XPT2046, I'm done talking to you"
    delayMicroseconds(100);    delayMicroseconds(50);
    delayMicroseconds(100);    
    delayMicroseconds(100);    // ===== STEP 6: Process the data =====
    delayMicroseconds(100);    // The XPT2046 sends 12 bits of data in this format:
    delayMicroseconds(100);    // Byte1: [D11 D10 D9 D8 D7 D6 D5 D4]
    delayMicroseconds(100);    // Byte2: [D3  D2  D1 D0  0  0  0  0]
    delayMicroseconds(100);    //
    delayMicroseconds(100);    // We combine them into 16 bits, then shift right by 3 to get the 12-bit value
    delayMicroseconds(100);    
    delayMicroseconds(100);    result = ((uint16_t)byte1 << 8) | byte2;  // Combine: 0bXXXXXXXXYYYY0000
    delayMicroseconds(100);    result = result >> 3;  // Shift right 3: 0b0000XXXXXXXXXXXX (now it's 12-bit)
    delayMicroseconds(100);    
    delayMicroseconds(100);    // Debug output (you can remove this later)
    delayMicroseconds(100);    Serial.print("CMD: 0x"); Serial.print(command, HEX);
    delayMicroseconds(100);    Serial.print(" B1: 0x"); Serial.print(byte1, HEX);
    delayMicroseconds(100);    Serial.print(" B2: 0x"); Serial.print(byte2, HEX);
    delayMicroseconds(100);    Serial.print(" → Result: ");
    delayMicroseconds(100);    Serial.println(result);
    delayMicroseconds(100);    
        return result;
    }
};

// Global touch object
TouchController touch;

#endif