/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_it.h"
#include "FreeRTOS.h"
#include "task.h"

/* FreeRTOS port 层函数 — 由 port.c 提供 */
extern void vPortSVCHandler(void);
extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);

/* External variables --------------------------------------------------------*/
extern DMA_HandleTypeDef hdma_adc1;
extern TIM_HandleTypeDef htim2;
extern UART_HandleTypeDef huart3;

/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
   while (1)
  {
  }
}

/**
  * @brief HardFault handler — blink PB5 fast, halt
  */
void HardFault_Handler(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    GPIOB->CRL &= ~(0xF << (5 * 4));
    GPIOB->CRL |= (0x3 << (5 * 4));
    while (1) {
        GPIOB->BSRR = GPIO_BSRR_BS5;
        for (volatile uint32_t i = 0; i < 200000; i++);
        GPIOB->BSRR = GPIO_BSRR_BR5;
        for (volatile uint32_t i = 0; i < 200000; i++);
    }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
/*
 * SVC_Handler: MUST use B (tail branch) not BL.
 * vPortSVCHandler relies on LR (=EXC_RETURN) to return from exception.
 */
__asm void SVC_Handler(void)
{
		IMPORT  vPortSVCHandler
		B       vPortSVCHandler           ; tail branch, LR preserved
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{

}

/**
  * @brief This function handles Pendable request for system service.
  */
/*
 * PendSV_Handler: MUST use B (tail branch) not BL.
 * xPortPendSVHandler also uses LR as EXC_RETURN.
 */
__asm void PendSV_Handler(void)
{
		IMPORT  xPortPendSVHandler
		B       xPortPendSVHandler         ; tail branch, LR preserved
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
	if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
	{
			xPortSysTickHandler();
	}
}

/**
  * @brief This function handles DMA1 channel1 global interrupt.
  */
void DMA1_Channel1_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_adc1);
}

/**
  * @brief This function handles TIM2 global interrupt.
  */
void TIM2_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim2);
}

/**
  * @brief This function handles USART3 global interrupt.
  */
void USART3_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart3);
}
