#include <sysinit.h>
#include <string.h>
#include <stdio.h>

#include "basic/basic.h"
#include "basic/config.h"

#include "lcd/render.h"
#include "lcd/print.h"

#include "core/i2c/i2c.h"
#include "filesystem/ff.h"

#include "usetable.h"

/***********************************************************************
* Monitor/Log temperatur (°C) and pressure (Pa) with your r0ket.
* You need a BMP085 sensor
* (http://www.bosch-sensortec.com/content/language1/html/3477.htm).
* For example on a board like this: https://www.sparkfun.com/products/9694
*
* Use "python3 tools/BMP085/altitude.py BMP085.LOG 10"
* for calc. altitude (m) (if you logged the data with ENABLE_LOGGING).
*
* With code snippets from http://bildr.org/2011/06/bmp085-arduino/
*
* 2012-06 Ikarus
***********************************************************************/


// Config.
#define BMP085_READ 0xEF        // I2C address of BMP085 (read modus)
#define BMP085_WRITE 0xEE       // I2C addres of BMP085 (write modus)
const uint8_t OSS = 0;          // Oversampling Setting (0..3)
#define LOG_FILE "BMP085.log"   // Name of the logfile.
        // (Delete if allready exists.)
        // Format: "temperatur, pressure" (temp in 0.1°C, pressure in Pa)
#define ENABLE_LOGGING 0        // 0 = no loggin, 1 = loggin on
#define REFRESH_INTERVAL 68     // Get new values (and log) every X ms.
        // + 10ms or more depending on OSS config for reading the values.
        // + Xms for logging to file (with write error check).
        // + Xms for displaying values.
        // 68 = ~10 logs per second.


// Global vars. (Calibration data).
int16_t ac1;
int16_t ac2;
int16_t ac3;
uint16_t ac4;
uint16_t ac5;
uint16_t ac6;
int16_t b1;
int16_t b2;
int16_t mb;
int16_t mc;
int16_t md;
// b5 is calculated in bmp085GetTemperature(...), this variable
// is also used in bmp085GetPressure(...)
// so ...Temperature(...) must be called before ...Pressure(...).
int16_t b5;
// For use in mainLoop() and logData().
int32_t temp = 0;
int32_t pres = 0;
FIL file;

// Prototypes.
void mainLoop();
uint8_t logData();
void bmp085Calibration();
void bmp085Convert(int32_t* temperature, int32_t* pressure); // Compensate.
int16_t bmp085ReadTemp(); // Uncompensated.
int32_t bmp085ReadPressure(); // Uncompensated.
int16_t bmp085ReadInt16(uint8_t address); // Read 2 bytes.
int32_t bmp085ReadInt24(uint8_t address); // Read 3 bytes.
// Subfunc for logData.
uint8_t checkError(UINT written, uint16_t size, FRESULT res);
uint8_t* itoa(int32_t value, uint8_t* result, uint8_t base);



void ram(void) {
    // Init.
    i2cInit(I2CMASTER);
    bmp085Calibration();

    // Print boot splash.
    lcdClear();
    lcdPrintln("");
    lcdPrintln("  BMP085");
    lcdPrintln("    Monitor");
    lcdPrintln("    Logger");
    lcdPrintln("");
    lcdPrintln("Up: Reset Max");
    lcdPrintln("Dwn: Rst. Min");
    lcdPrintln("Left: Exit");
    lcdRefresh();
    delayms(1500);
    mainLoop();
}

// Main loop.
void mainLoop() {
    // Init Min/Max.
    bmp085Convert(&temp, &pres);
    int32_t max_temp = temp;
    int32_t max_pres = pres;
    int32_t min_temp = temp;
    int32_t min_pres = pres;

    FRESULT res;

    // Open logfile.
    if (ENABLE_LOGGING) {
        res = f_open(&file, LOG_FILE, FA_CREATE_ALWAYS|FA_WRITE);
        if (res) {
            lcdClear();
            lcdPrintln("File Error");
            lcdPrintln("(f_open)");
            lcdRefresh();
            delayms(2000);
            return;
        }
    }

    do{
        // Get new values from sensor.
        bmp085Convert(&temp, &pres);
        // Check for new max. or min.
        if (temp > max_temp)
            max_temp = temp;
        else if (temp < min_temp)
            min_temp = temp;
        if (pres > max_pres)
            max_pres = pres;
        else if (pres < min_pres)
            min_pres = pres;

        if (ENABLE_LOGGING)
            if (logData())
                return; // Data error.


        lcdClear();
        lcdPrintln("Temperature:");
        lcdPrint(" ");
        lcdPrint(IntToStr(temp/10, 2, 0));
        lcdPrint(".");
        lcdPrint(IntToStr(temp%10, 1, F_ZEROS));
        lcdPrintln("C");
        lcdPrint(" Max ");
        lcdPrint(IntToStr(max_temp/10, 2, 0));
        lcdPrint(".");
        lcdPrint(IntToStr(max_temp%10, 1, F_ZEROS));
        lcdPrintln("C");
        lcdPrint(" Min ");
        lcdPrint(IntToStr(min_temp/10, 2, 0));
        lcdPrint(".");
        lcdPrint(IntToStr(min_temp%10, 1, F_ZEROS));
        lcdPrintln("C");

        lcdPrintln("Pressure:");
        lcdPrint(" ");
        lcdPrint(IntToStr(pres, 8, 0));
        lcdPrintln("Pa");
        lcdPrint(" Max ");
        lcdPrint(IntToStr(max_pres, 8, 0));
        lcdPrintln("Pa");
        lcdPrint(" Min ");
        lcdPrint(IntToStr(min_pres, 8, 0));
        lcdPrintln("Pa");
        lcdRefresh();

        // Handle input.
        switch(getInputRaw()) {
            case BTN_UP:
                max_temp = temp;
                max_pres = pres;
                break;
            case BTN_DOWN:
                min_pres = pres;
                min_temp = temp;
                break;
        }
    } while ((getInputWaitTimeout(REFRESH_INTERVAL))!=BTN_LEFT);
    // Close logfile.
    if (ENABLE_LOGGING)
        f_close(&file);
}

// Log data to file.
uint8_t logData() {
    uint8_t ret = 0;
    FRESULT res;
    UINT written = 0;
    uint8_t buf1[15];
    uint8_t buf2[15];
    uint8_t seperator[2] = {',', ' '};
    uint8_t end[1] = {'\n'};

    itoa(temp, buf1, 10);
    itoa(pres, buf2, 10);

    res = f_write(&file, buf1, strlen((char *)buf1), &written);
    if ((ret=checkError(written, strlen((char *)buf1), res)))
        return ret;
    res = f_write(&file, seperator, 2, &written);
    if ((ret=checkError(written, 2, res)))
        return ret;
    res = f_write(&file, buf2, strlen((char *)buf2), &written);
    if ((ret=checkError(written, strlen((char *)buf2), res)))
        return ret;
    res = f_write(&file, end, 1, &written);
    if ((checkError(written, 1, res)))
        return ret;
    return 0;
}

uint8_t checkError(UINT written, uint16_t size, FRESULT res) {
    if(res) {
        lcdClear();
        lcdPrintln("File Error");
        lcdPrintln("(f_write)");
        lcdRefresh();
        delayms(2000);
        return -1;
    }
    if( written != size ) {
        lcdClear();
        lcdPrintln("Error while");
        lcdPrintln("writing (size)");
        lcdRefresh();
        delayms(2000);
        return -2;
    }
    return 0;
}

// Stores all of the bmp085's calibration values into global variables.
// Calibration values are required to calculate temp and pressure.
void bmp085Calibration() {
    ac1 = bmp085ReadInt16(0xAA);
    ac2 = bmp085ReadInt16(0xAC);
    ac3 = bmp085ReadInt16(0xAE);
    ac4 = bmp085ReadInt16(0xB0);
    ac5 = bmp085ReadInt16(0xB2);
    ac6 = bmp085ReadInt16(0xB4);
    b1 = bmp085ReadInt16(0xB6);
    b2 = bmp085ReadInt16(0xB8);
    mb = bmp085ReadInt16(0xBA);
    mc = bmp085ReadInt16(0xBC);
    md = bmp085ReadInt16(0xBE);
}

// Convert the sensor values to human readable valuse.
// (temperature to 0.1°C, pressure to Pa.)
void bmp085Convert(int32_t* temperature, int32_t* pressure) {
    int32_t ut;
    int32_t up;
    int32_t x1, x2, b5, b6, x3, b3, p;
    uint32_t b4, b7;

    ut = bmp085ReadTemp();
    // Some bug here, have to read twice to get good data.
    // (Taken from a tutorial, but I got good data by reading once!)
    //~ ut = bmp085ReadTemp();
    up = bmp085ReadPressure();
    //~ up = bmp085ReadPressure();

    x1 = ((int32_t)ut - ac6) * ac5 >> 15;
    x2 = ((int32_t) mc << 11) / (x1 + md);
    b5 = x1 + x2;
    *temperature = (b5 + 8) >> 4;

    b6 = b5 - 4000;
    x1 = (b2 * (b6 * b6 >> 12)) >> 11;
    x2 = ac2 * b6 >> 11;
    x3 = x1 + x2;
    b3 = (((int32_t) ac1 * 4 + x3) + 2)/4;
    x1 = ac3 * b6 >> 13;
    x2 = (b1 * (b6 * b6 >> 12)) >> 16;
    x3 = ((x1 + x2) + 2) >> 2;
    b4 = (ac4 * (uint32_t) (x3 + 32768)) >> 15;
    b7 = ((uint32_t) up - b3) * (50000 >> OSS);
    p = b7 < 0x80000000 ? (b7 * 2) / b4 : (b7 / b4) * 2;
    x1 = (p >> 8) * (p >> 8);
    x1 = (x1 * 3038) >> 16;
    x2 = (-7357 * p) >> 16;
    *pressure = p + ((x1 + x2 + 3791) >> 4);
}


int16_t bmp085ReadTemp() {
    // Write 0x2E into Register 0xF4.
    // This requests a temperature reading.
    I2CMasterBuffer[0] = BMP085_WRITE;
    I2CMasterBuffer[1] = 0xF4;
    I2CMasterBuffer[2] = 0x2E;
    I2CWriteLength = 3;
    I2CReadLength = 0;
    i2cEngine();

    // Wait at least 4.5ms
    delayms(5);

    // Read two bytes from registers 0xF6 and 0xF7.
    return bmp085ReadInt16(0xF6);
}

int32_t bmp085ReadPressure() {
    // Write 0x34+(OSS<<6) into register 0xF4.
    // Request a pressure reading w/ oversampling setting.
    I2CMasterBuffer[0] = BMP085_WRITE;
    I2CMasterBuffer[1] = 0xF4;
    I2CMasterBuffer[2] = 0x34 + (OSS<<6);
    I2CWriteLength = 3;
    I2CReadLength = 0;
    i2cEngine();

    // Wait for conversion, delay time dependent on OSS.
    delayms(2 + (3<<OSS));

    // Read register 0xF6 (MSB), 0xF7 (LSB), and 0xF8 (XLSB).
    return bmp085ReadInt24(0xF6);
}

// Read 2 bytes from the BMP085.
// First byte will be from 'address'.
// Second byte will be from 'address'+1.
int16_t bmp085ReadInt16(uint8_t address) {
    // Address module (BMP085) and register (address).
    I2CMasterBuffer[0] = BMP085_WRITE;
    I2CMasterBuffer[1] = address;
    I2CWriteLength = 2;
    I2CReadLength = 0;
    i2cEngine();

    // Get Data from address.
    I2CMasterBuffer[0] = BMP085_READ;
    I2CWriteLength = 1;
    I2CReadLength = 2; // Read 2 bytes.
    i2cEngine();
    return (int16_t)I2CSlaveBuffer[0] << 8 | I2CSlaveBuffer[1];
}

// Read 3 bytes from the BMP085.
// First byte will be from 'address'.
// Second byte will be from 'address'+1 and so on.
int32_t bmp085ReadInt24(uint8_t address) {
    // Address module (BMP085) and register (address).
    I2CMasterBuffer[0] = BMP085_WRITE;
    I2CMasterBuffer[1] = address;
    I2CWriteLength = 2;
    I2CReadLength = 0;
    i2cEngine();

    // Get Data from address.
    I2CMasterBuffer[0] = BMP085_READ;
    I2CWriteLength = 1;
    I2CReadLength = 3; // Read 3 bytes.
    i2cEngine();
    return (((int32_t) I2CSlaveBuffer[0] << 16) |
        ((int32_t) I2CSlaveBuffer[1] << 8) |
        (int32_t) I2CSlaveBuffer[2]) >> (8-OSS);
}

// C++ version 0.4 char* style "itoa":
// Written by Lukás Chmela
// Released under GPLv3.
// (For loggin in to file.)
uint8_t* itoa(int32_t value, uint8_t* result, uint8_t base) {
    // Check that the base if valid.
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    uint8_t* ptr = result, *ptr1 = result, tmp_char;
    int32_t tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign.
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}
