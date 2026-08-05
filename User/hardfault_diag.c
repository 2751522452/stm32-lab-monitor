/**
  * @brief  HardFault 诊断模块
  *
  * 由 stm32f1xx_it.c 中的 HardFault_Handler 调用。
  * 通过 USART1 打印故障寄存器 + 栈帧内容，然后闪烁 PB5 报死。
  */

#include "stm32f1xx_hal.h"
#include "hardfault_diag.h"
#include <stdint.h>

/* ---- Handler 进入跟踪计数器 ---- */
volatile uint32_t trace_svc_cnt      = 0;
volatile uint32_t trace_pendsv_cnt   = 0;
volatile uint32_t trace_systick_cnt  = 0;
volatile uint32_t trace_systick_freertos_cnt = 0;

/* ------------------------------------------------------------------ */
/* USART1 直接操作（不依赖 HAL / printf）                             */
/* ------------------------------------------------------------------ */

/* 使用 CMSIS 的 USART1 外设指针，不自定义宏 — 避免重定义警告 */

static void uart1_putc(char c)
{
    while ((USART1->SR & USART_SR_TXE) == 0) { }
    USART1->DR = (uint32_t)(uint8_t)c;
}

static void uart1_puts(const char *s)
{
    while (*s) uart1_putc(*s++);
}

static const char hex_chars[] = "0123456789ABCDEF";

static void uart1_puthex8(uint32_t val)
{
    for (int i = 7; i >= 0; i--)
        uart1_putc(hex_chars[(val >> (i * 4)) & 0xF]);
}

static void uart1_puthex2(uint8_t val)
{
    uart1_putc(hex_chars[(val >> 4) & 0xF]);
    uart1_putc(hex_chars[val & 0xF]);
}

/* ------------------------------------------------------------------ */
/* 诊断入口                                                           */
/* ------------------------------------------------------------------ */

void HardFault_Dump(uint32_t *sp, uint32_t exc_return)
{
    uint32_t stacked[8];   /* R0,R1,R2,R3,R12,LR,PC,xPSR */

    stacked[0] = sp[0];    /* R0  */
    stacked[1] = sp[1];    /* R1  */
    stacked[2] = sp[2];    /* R2  */
    stacked[3] = sp[3];    /* R3  */
    stacked[4] = sp[4];    /* R12 */
    stacked[5] = sp[5];    /* LR  */
    stacked[6] = sp[6];    /* PC  */
    stacked[7] = sp[7];    /* xPSR */

    uint32_t fault_pc = stacked[6];

    /* 读取 SCB 故障寄存器 */
    uint32_t cfsr = SCB->CFSR;
    uint32_t hfsr = SCB->HFSR;
    uint32_t mmfar = SCB->MMFAR;
    uint32_t bfar  = SCB->BFAR;

    uint8_t  mmfsr = (uint8_t)(cfsr & 0xFF);
    uint8_t  bfsr  = (uint8_t)((cfsr >> 8) & 0xFF);
    uint16_t ufsr  = (uint16_t)((cfsr >> 16) & 0xFFFF);

    /* ---- 输出 ---- */
    uart1_puts("\r\n\r\n========================================\r\n");
    uart1_puts("        HARDFAULT DIAGNOSTICS\r\n");
    uart1_puts("========================================\r\n\r\n");

    uart1_puts("[Fault Type]\r\n");
    uart1_puts("  HFSR = 0x"); uart1_puthex8(hfsr);
    if (hfsr & (1U << 30)) uart1_puts("  [FORCED]\r\n");
    if (hfsr & (1U << 1))  uart1_puts("  [VECTTBL]\r\n");

    uart1_puts("  CFSR = 0x"); uart1_puthex8(cfsr); uart1_puts("\r\n");

    /* UsageFault */
    uart1_puts("    UFSR  = 0x");
    uart1_puthex2((uint8_t)(ufsr >> 8)); uart1_puthex2((uint8_t)(ufsr));
    if (ufsr & (1U << 9)) uart1_puts("  [DIVBYZERO]\r\n");
    if (ufsr & (1U << 8)) uart1_puts("  [UNALIGNED]\r\n");
    if (ufsr & (1U << 3)) uart1_puts("  [NOCP]\r\n");
    if (ufsr & (1U << 2)) uart1_puts("  [INVPC]\r\n");
    if (ufsr & (1U << 1)) uart1_puts("  [INVSTATE]\r\n");
    if (ufsr & (1U << 0)) uart1_puts("  [UNDEFINSTR]\r\n");

    /* BusFault */
    uart1_puts("    BFSR  = 0x"); uart1_puthex2(bfsr);
    if (bfsr & (1U << 7)) uart1_puts("  [BFARVALID]\r\n");
    if (bfsr & (1U << 5)) uart1_puts("  [LSPERR]\r\n");
    if (bfsr & (1U << 4)) uart1_puts("  [STKERR]\r\n");
    if (bfsr & (1U << 3)) uart1_puts("  [UNSTKERR]\r\n");
    if (bfsr & (1U << 2)) uart1_puts("  [IMPRECISERR]\r\n");
    if (bfsr & (1U << 1)) uart1_puts("  [PRECISERR]\r\n");
    if (bfsr & (1U << 0)) uart1_puts("  [IBUSERR]\r\n");

    /* MemManage */
    uart1_puts("    MMFSR = 0x"); uart1_puthex2(mmfsr);
    if (mmfsr & (1U << 7)) uart1_puts("  [MMARVALID]\r\n");
    if (mmfsr & (1U << 5)) uart1_puts("  [MLSPERR]\r\n");
    if (mmfsr & (1U << 4)) uart1_puts("  [MSTKERR]\r\n");
    if (mmfsr & (1U << 3)) uart1_puts("  [MUNSTKERR]\r\n");
    if (mmfsr & (1U << 1)) uart1_puts("  [DACCVIOL]\r\n");
    if (mmfsr & (1U << 0)) uart1_puts("  [IACCVIOL]\r\n");

    /* 故障地址 */
    uart1_puts("\r\n[Fault Addresses]\r\n");
    uart1_puts("  BFAR  = 0x"); uart1_puthex8(bfar);  uart1_puts("\r\n");
    uart1_puts("  MMFAR = 0x"); uart1_puthex8(mmfar); uart1_puts("\r\n");

    /* 栈帧 */
    uart1_puts("\r\n[Stack Frame]  EXC_RETURN = 0x");
    uart1_puthex8(exc_return);
    uart1_puts((exc_return & (1U << 2)) ? "  (PSP)\r\n" : "  (MSP)\r\n");

    uart1_puts("  SP    = 0x"); uart1_puthex8((uint32_t)sp);     uart1_puts("\r\n");
    uart1_puts("  PC    = 0x"); uart1_puthex8(stacked[6]);       uart1_puts("\r\n");
    uart1_puts("  LR    = 0x"); uart1_puthex8(stacked[5]);       uart1_puts("\r\n");
    uart1_puts("  R0    = 0x"); uart1_puthex8(stacked[0]);       uart1_puts("\r\n");
    uart1_puts("  R1    = 0x"); uart1_puthex8(stacked[1]);       uart1_puts("\r\n");
    uart1_puts("  R2    = 0x"); uart1_puthex8(stacked[2]);       uart1_puts("\r\n");
    uart1_puts("  R3    = 0x"); uart1_puthex8(stacked[3]);       uart1_puts("\r\n");
    uart1_puts("  R12   = 0x"); uart1_puthex8(stacked[4]);       uart1_puts("\r\n");
    uart1_puts("  xPSR  = 0x"); uart1_puthex8(stacked[7]);       uart1_puts("\r\n");

    /* ---- Handler 进入跟踪 ---- */
    uart1_puts("\r\n[Handler Trace]\r\n");
    uart1_puts("  SVC_Handler   entered = "); uart1_puthex8(trace_svc_cnt); uart1_puts("\r\n");
    uart1_puts("  PendSV_Handler entered = "); uart1_puthex8(trace_pendsv_cnt); uart1_puts("\r\n");
    uart1_puts("  SysTick_Handler entered = "); uart1_puthex8(trace_systick_cnt); uart1_puts("\r\n");
    uart1_puts("  SysTick -> xPortSysTickHandler = "); uart1_puthex8(trace_systick_freertos_cnt); uart1_puts("\r\n");

    uart1_puts("\r\n[Info]\r\n");
    uart1_puts("  Fault PC = 0x"); uart1_puthex8(fault_pc);
    uart1_puts("\r\n  -> Check .map file for this address\r\n");

    uart1_puts("\r\n========================================\r\n");
    uart1_puts("  System Halted\r\n");
    uart1_puts("========================================\r\n\r\n");

    /* 无论 UART 是否可用，都闪 LED 报死 */
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    GPIOB->CRL &= ~(0xF << (5 * 4));
    GPIOB->CRL |= (0x3 << (5 * 4));
    for (;;) {
        GPIOB->BSRR = GPIO_BSRR_BS5;
        for (volatile uint32_t i = 0; i < 200000; i++);
        GPIOB->BSRR = GPIO_BSRR_BR5;
        for (volatile uint32_t i = 0; i < 200000; i++);
    }
}
