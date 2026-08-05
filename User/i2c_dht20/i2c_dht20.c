#include "i2c_dht20/i2c_dht20.h"

extern I2C_HandleTypeDef hi2c1;

#define DHT20_ADDR (0x38 << 1)

static uint8_t DHT20_WaitReady(void)
{
    uint8_t status = 0;
    uint32_t timeout = 0;

    do
    {
        HAL_I2C_Master_Receive(&hi2c1, DHT20_ADDR, &status, 1, 10);
        HAL_Delay(2);
        timeout++;
        if(timeout > 50) return 1;
    }
    while(status & 0x80);

    return 0;
}

uint8_t DHT20_Init(void)
{
    HAL_Delay(100);

    // 上电初始化（固定写法）
    uint8_t init_cmd[3] = {0xAC, 0x33, 0x00};
    HAL_I2C_Master_Transmit(&hi2c1, DHT20_ADDR, init_cmd, 3, 100);

    HAL_Delay(80);

    return 0;
}

uint8_t DHT20_Read(DHT20_Data *out)
{
    uint8_t cmd[3] = {0xAC, 0x33, 0x00};
    uint8_t data[6];

    // 1. 触发测量
    if(HAL_I2C_Master_Transmit(&hi2c1, DHT20_ADDR, cmd, 3, 100) != HAL_OK)
        return 1;

    // 2. 等待完成
    if(DHT20_WaitReady())
        return 2;

    // 3. 读取数据
    if(HAL_I2C_Master_Receive(&hi2c1, DHT20_ADDR, data, 6, 100) != HAL_OK)
        return 3;

    // 4. 解析 20bit 湿度 + 20bit 温度
    uint32_t raw_humi =
        ((uint32_t)data[1] << 12) |
        ((uint32_t)data[2] << 4)  |
        (data[3] >> 4);

    uint32_t raw_temp =
        ((uint32_t)(data[3] & 0x0F) << 16) |
        ((uint32_t)data[4] << 8) |
        data[5];

    // 5. 转换
    out->humidity = (raw_humi * 100.0f) / 1048576.0f;
    out->temperature = (raw_temp * 200.0f / 1048576.0f) - 50.0f;

    return 0;
}
