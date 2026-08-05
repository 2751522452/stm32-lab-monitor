#include "i2c_scan/i2c_scan.h"
#include "i2c/i2c.h"
#include "stdio.h"

extern I2C_HandleTypeDef hi2c1;

void I2C_Scan(void)
{
    for(uint8_t addr = 1; addr < 127; addr++)
    {
        if(HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 10) == HAL_OK)
        {
            printf("Found I2C device: 0x%02X\r\n", addr);
        }
    }
}
