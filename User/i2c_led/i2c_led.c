#include "i2c_led.h"
#include "i2c/i2c.h"
#include "fonts/fonts.h"

extern I2C_HandleTypeDef hi2c1;

#define OLED_ADDR (0x3C << 1)

/**
 * @brief  向SSD1306发送命令
 */
void OLED_WriteCmd(uint8_t cmd)
{
    uint8_t buf[2];

    buf[0] = 0x00;      // Control Byte：命令
    buf[1] = cmd;

    HAL_I2C_Master_Transmit(&hi2c1,OLED_ADDR,buf,2,HAL_MAX_DELAY);
}

/**
 * @brief  向SSD1306发送数据
 */
void OLED_WriteData(uint8_t data)
{
    uint8_t buf[2];

    buf[0] = 0x40;      // Control Byte：数据
    buf[1] = data;

    HAL_I2C_Master_Transmit(&hi2c1,OLED_ADDR,buf,2,HAL_MAX_DELAY);
}

void OLED_Init(void)
{
    HAL_Delay(100);

    OLED_WriteCmd(0xAE); // display off

		OLED_WriteCmd(0x20); // memory mode
		OLED_WriteCmd(0x02); // page mode

		OLED_WriteCmd(0xB0);

		OLED_WriteCmd(0xA1); // SEG remap（关键：左右修正）
		OLED_WriteCmd(0xC8); // COM scan（关键：上下修正）

		OLED_WriteCmd(0x00);
		OLED_WriteCmd(0x10);

		OLED_WriteCmd(0x40);

		OLED_WriteCmd(0x81);
		OLED_WriteCmd(0xFF);

		OLED_WriteCmd(0xA6); // normal display

		OLED_WriteCmd(0xA8);
		OLED_WriteCmd(0x3F);

		OLED_WriteCmd(0xD3);
		OLED_WriteCmd(0x00);

		OLED_WriteCmd(0xD5);
		OLED_WriteCmd(0x80);

		OLED_WriteCmd(0xD9);
		OLED_WriteCmd(0xF1);

		OLED_WriteCmd(0xDA);
		OLED_WriteCmd(0x12);

		OLED_WriteCmd(0xDB);
		OLED_WriteCmd(0x40);

		OLED_WriteCmd(0x8D);
		OLED_WriteCmd(0x14);

		OLED_WriteCmd(0xAF); // display on
}

void OLED_Clear(void)
{
    for(uint8_t page = 0; page < 8; page++)
    {
        OLED_WriteCmd(0xB0 + page);
        OLED_WriteCmd(0x00);
        OLED_WriteCmd(0x10);

        for(uint8_t col = 0; col < 128; col++)
        {
            OLED_WriteData(0x00);
        }
    }
}

void OLED_Fill(uint8_t data)
{
    uint8_t page, col;

    for(page = 0; page < 8; page++)
    {
        // 选择页地址
        OLED_WriteCmd(0xB0 + page);

        // 列地址低4位
        OLED_WriteCmd(0x00);

        // 列地址高4位
        OLED_WriteCmd(0x10);

        for(col = 0; col < 128; col++)
        {
            OLED_WriteData(data);
        }
    }
}

void OLED_SetCursor(uint8_t page, uint8_t col)
{
    OLED_WriteCmd(0xB0 + page);
    OLED_WriteCmd(0x00 + (col & 0x0F));
    OLED_WriteCmd(0x10 + (col >> 4));
}

void OLED_ShowChar(uint8_t x, uint8_t y, char ch)
{
    uint8_t index = ch - ' ';

    OLED_SetCursor(y, x);

    for(uint8_t i = 0; i < 6; i++)
    {
        OLED_WriteData(Font6x8[index][i]);
    }

    // 关键：空1列作为间距
    OLED_WriteData(0x00);
}

void OLED_ShowString(uint8_t x, uint8_t page, const char *str)
{
    while (*str)
    {
        OLED_ShowChar(x, page, *str);

        x += 6;  // 字符宽度 + 间距

        if (x > 122)
        {
            x = 0;
            page++;

            if (page > 7)
                return;
        }

        str++;
    }
}

void OLED_UpdateLine(uint8_t page, char *str)
{
    OLED_WriteCmd(0xB0 + page);
    OLED_WriteCmd(0x00);
    OLED_WriteCmd(0x10);

    for (int i = 0; i < 16; i++)
    {
        if (*str)
            OLED_WriteData(Font6x8[*str - ' '][i % 6]);
        else
            OLED_WriteData(0x00);

        str++;
    }
}
