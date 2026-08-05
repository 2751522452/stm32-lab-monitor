#include "spi_w25q64/spi_w25q64.h"

extern SPI_HandleTypeDef hspi2;

/*-------------------- W25Q64命令 --------------------*/
#define W25Q64_CMD_WRITE_ENABLE    0x06
#define W25Q64_CMD_READ_STATUS1    0x05
#define W25Q64_CMD_READ_ID         0x9F
#define W25Q64_CMD_PAGE_PROGRAM    0x02
#define W25Q64_CMD_READ_DATA       0x03
#define W25Q64_CMD_SECTOR_ERASE    0x20

/*-------------------- CS控制 --------------------*/
#define W25Q64_CS_LOW()   HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET)
#define W25Q64_CS_HIGH()  HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET)

/*-------------------- SPI发送接收 --------------------*/
static uint8_t W25Q64_SPI_TransmitReceive(uint8_t data)
{
    uint8_t rx;
    HAL_SPI_TransmitReceive(&hspi2,&data,&rx,1,HAL_MAX_DELAY);

    return rx;
}

//初始化
void W25Q64_Init(void)
{
    W25Q64_CS_HIGH();
}

//写使能
void W25Q64_WriteEnable(void)
{
    W25Q64_CS_LOW();

    W25Q64_SPI_TransmitReceive(W25Q64_CMD_WRITE_ENABLE);

    W25Q64_CS_HIGH();
}

//读取 JEDEC ID
uint32_t W25Q64_ReadID(void)
{
    uint8_t manufacturer;
    uint8_t memory_type;
    uint8_t capacity;

    W25Q64_CS_LOW();

    W25Q64_SPI_TransmitReceive(W25Q64_CMD_READ_ID);

    manufacturer = W25Q64_SPI_TransmitReceive(0xFF);
    memory_type  = W25Q64_SPI_TransmitReceive(0xFF);
    capacity     = W25Q64_SPI_TransmitReceive(0xFF);

    W25Q64_CS_HIGH();

    return ((uint32_t)manufacturer << 16) |
           ((uint32_t)memory_type << 8) |
            capacity;
}

// 读取 W25Q64 的状态
uint8_t W25Q64_ReadStatus1(void)
{
    uint8_t status;

    W25Q64_CS_LOW();

    W25Q64_SPI_TransmitReceive(W25Q64_CMD_READ_STATUS1);

    status = W25Q64_SPI_TransmitReceive(0xFF);

    W25Q64_CS_HIGH();

    return status;
}

// 等待 Busy
void W25Q64_WaitBusy(void)
{
    while(W25Q64_ReadStatus1() & 0x01)
    {
    }
}

//擦除
void W25Q64_SectorErase(uint32_t addr)
{
    /* 写使能 */
    W25Q64_WriteEnable();

    /* 拉低CS */
    W25Q64_CS_LOW();

    /* 发送擦除命令 */
    W25Q64_SPI_TransmitReceive(W25Q64_CMD_SECTOR_ERASE);

    /* 发送24位地址 */
    W25Q64_SPI_TransmitReceive((addr >> 16) & 0xFF);
    W25Q64_SPI_TransmitReceive((addr >> 8) & 0xFF);
    W25Q64_SPI_TransmitReceive(addr & 0xFF);

    /* 拉高CS */
    W25Q64_CS_HIGH();

    /* 等待擦除完成 */
    W25Q64_WaitBusy();
}

//读取
void W25Q64_Read(uint32_t addr,uint8_t *buf,uint16_t len)
{
    W25Q64_CS_LOW();

    W25Q64_SPI_TransmitReceive(W25Q64_CMD_READ_DATA);

    W25Q64_SPI_TransmitReceive((addr >> 16) & 0xFF);
    W25Q64_SPI_TransmitReceive((addr >> 8) & 0xFF);
    W25Q64_SPI_TransmitReceive(addr & 0xFF);

    while(len--)
    {
        *buf++ = W25Q64_SPI_TransmitReceive(0xFF);
    }

    W25Q64_CS_HIGH();
}

//页编程
void W25Q64_PageProgram(uint32_t addr,
                        uint8_t *buf,
                        uint16_t len)
{
    W25Q64_WriteEnable();

    W25Q64_CS_LOW();

    W25Q64_SPI_TransmitReceive(W25Q64_CMD_PAGE_PROGRAM);

    W25Q64_SPI_TransmitReceive((addr >> 16) & 0xFF);
    W25Q64_SPI_TransmitReceive((addr >> 8) & 0xFF);
    W25Q64_SPI_TransmitReceive(addr & 0xFF);

    while(len--)
    {
        W25Q64_SPI_TransmitReceive(*buf++);
    }

    W25Q64_CS_HIGH();

    W25Q64_WaitBusy();
}

//写入
void W25Q64_Write(uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint32_t page_remain;

    while(len > 0)
    {
        /* 当前页剩余空间 */
        page_remain = 256 - (addr % 256);

        /* 本次写入长度 */
        if(len < page_remain)
        {
            page_remain = len;
        }

        /* 写一页（不会跨页） */
        W25Q64_PageProgram(addr, buf, page_remain);

        /* 更新地址 */
        addr += page_remain;

        /* 更新数据指针 */
        buf += page_remain;

        /* 更新剩余长度 */
        len -= page_remain;
    }
}
