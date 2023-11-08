
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2022 STMicroelectronics International N.V.
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f0xx_hal.h"
#include "usb_device.h"

/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include "slcan/slcan.h"
#include "slcan/slcan_additional.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan;
TIM_HandleTypeDef htim3;
IWDG_HandleTypeDef hiwdg;
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_tx;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_spi1_tx;
/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
CanTxMsgTypeDef CanTxBuffer;
CanRxMsgTypeDef CanRxBuffer;

uart_buffer uart_buffers[2];
volatile tcanRx canRxFlags;
volatile int32_t serialNumber;
const uint32_t *uid = (uint32_t *)(UID_BASE + 4);
static uint8_t rxFullFlag = 0;
uint8_t msgPending = 0;
uint8_t activeBuffer = 0;
uint16_t counter = 0; //LED animation counter
uint32_t lastLEDTick = 0, lastAPATick = 0;
uint32_t errCode = 0;
uint8_t rxCnt = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);
static void MX_DMA_Init(void);
static void MX_IWDG_Init(void);
/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
void bootloaderSwitcher();
void handleCAN(void);
void pantherLights(void);
void toggleLED(void);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
 	serialNumber = TYPE_ID | (((*uid) << 8) & 0xFFFFFF00);
  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CAN_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  MX_USB_DEVICE_Init();

  //Disable WATCHDOG on debug
  __HAL_RCC_DBGMCU_CLK_ENABLE();
  __HAL_DBGMCU_FREEZE_IWDG();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */

  hcan.pTxMsg = &CanTxBuffer;
  hcan.pRxMsg = &CanRxBuffer;
  canRxFlags.flags.byte = 0;

// CAN RX setup
  slcanClearAllFilters();
  HAL_NVIC_SetPriority(CEC_CAN_IRQn, 2, 2);
  HAL_CAN_Receive_IT(&hcan, CAN_FIFO0);

// UART RX setup
//enable CR character detection and disable UART RX interrupt (on each byte)
  USART2->CR1 &= ~USART_CR1_UE;
  USART2->CR2 |= USART_CR2_ADDM7;
  USART2->CR2 |= (uint32_t)(0x0D << USART_CR2_ADD_Pos);
  USART2->CR1 |= USART_CR1_CMIE;
  USART2->CR1 &= ~USART_CR1_RXNEIE;
  HAL_Delay(1);
  USART2->CR1 |= USART_CR1_UE;

// start DMA
  HAL_UART_Receive_DMA(&huart2, uart_buffers[activeBuffer].data, UART_RX_BUFFER_SIZE); //UART
  HAL_SPI_Transmit_DMA(&hspi1, (uint8_t*)&strip_buffer, sizeof(struct _strip_buffer)); //SPI (led strip)
// enable UART global interrupts
  HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
  NVIC_EnableIRQ(USART2_IRQn);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  initCanOnStart();

  while (1)
  {
	  handleCAN();
	  pantherLights();
	  toggleLED();
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
	HAL_IWDG_Refresh(&hiwdg);
  }
  /* USER CODE END 3 */

}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

    /**Initializes the CPU, AHB and APB busses clocks
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;


  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 2, 2);
}

/* CAN init function */
static void MX_CAN_Init(void)
{

  hcan.Instance = CAN;
  hcan.Init.Prescaler = 3;
  hcan.Init.Mode = CAN_MODE_NORMAL;
  hcan.Init.SJW = CAN_SJW_2TQ;
  hcan.Init.BS1 = CAN_BS1_11TQ;
  hcan.Init.BS2 = CAN_BS2_4TQ;
  hcan.Init.TTCM = DISABLE;
  hcan.Init.ABOM = ENABLE;
  hcan.Init.AWUM = DISABLE;
  hcan.Init.NART = DISABLE;
  hcan.Init.RFLM = DISABLE;
  hcan.Init.TXFP = DISABLE;
  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
    _Error_Handler("CAN", 0);
  }

}

/* IWDG init function */
static void MX_IWDG_Init(void)
{

  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
  hiwdg.Init.Window = 4095;
  hiwdg.Init.Reload = 4095;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* USART2 init function */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 6000000;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
  huart2.Init.OverSampling = UART_OVERSAMPLING_8;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_ENABLE;

  //!!!!!!!!!!!!!!!!!! CHECK RX/TX PINS ON OVERLAY!!!!!!!!!!!!!!!!!!!!!!!!!

//  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_SWAP_INIT;
//  huart2.AdvancedInit.Swap = UART_ADVFEATURE_SWAP_ENABLE;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 2, 2);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
  /* DMA1_Channel4_5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_5_IRQn);

}

/** Configure pins
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CAN_MOD_GPIO_Port, CAN_MOD_Pin|LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : CAN_MOD_Pin */
  GPIO_InitStruct.Pin = CAN_MOD_Pin | LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CAN_MOD_GPIO_Port, &GPIO_InitStruct);

  	GPIO_InitStruct.Pin = GPIO_PIN_0;
  	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  	GPIO_InitStruct.Pull = GPIO_NOPULL;
  	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* SPI1 init function */
static void MX_SPI1_Init(void)
{

  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }
}

/* USER CODE BEGIN 4 */
void handleCAN(void)
{
	if (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED) //if USB is connected, check it not UART
	{
		slCanCheckCommand(command);
		if (canRxFlags.flags.byte != 0)
		{
			slcanReciveCanFrame(hcan.pRxMsg);
			canRxFlags.flags.fifo1 = 0;
			HAL_CAN_Receive_IT(&hcan, CAN_FIFO0);
		}
	}
	else
	{
		if(rxFullFlag == 0)
		{
			HAL_UART_Receive_DMA(&huart2, uart_buffers[activeBuffer].data, UART_RX_BUFFER_SIZE); //resume DMA if buffers cleared and ready for reception
		}
		if(rxFullFlag == 1)
		{
			rxFullFlag = 0;
			memset(uart_buffers[!activeBuffer].data, 0 , UART_RX_BUFFER_SIZE);
			uart_buffers[!activeBuffer].bufferCleared = 1;
//			__HAL_UART_FLUSH_DRREGISTER(&huart2); //clear the UART register
			slCanCheckCommand(command);
		}
		if (canRxFlags.flags.byte != 0 && hdma_usart2_tx.State == HAL_DMA_STATE_READY) //5 fix to uart tx buffer overwriting
		{
			slcanReciveCanFrame(hcan.pRxMsg);
			canRxFlags.flags.fifo1 = 0;
			HAL_CAN_Receive_IT(&hcan, CAN_FIFO0);
		}
	}

	if(hdma_usart2_tx.State == HAL_DMA_STATE_READY)
	{
		slcanOutputFlush(); //send data via UART or USB (if connected)
	}
}

void pantherLights(void)
{
	if(HAL_GetTick() - lastAPATick >= 15) // LED strip animation update (od Mikolaja W.)
	{
		strip_buffer.endFrame = 0;
		strip_buffer.startFrame = 0;

		for (uint8_t i = 0; i < NUM_LEDS; i++)
		{

		// Fade out colors
		  int r = strip_buffer.strip[i].fields.red - 3;
		  int g = strip_buffer.strip[i].fields.green - 3;
		  // Draw white lines
		  if (i == counter - 0 || NUM_LEDS - i == (counter - 200) * 2 || NUM_LEDS - i == (counter - 200) * 2 - 1) {
			r += 127;
			g += 127;
		  }

		  // Draw red lines
		  if (NUM_LEDS - i == (counter * 2) - 50 || NUM_LEDS - i == (counter * 2) - 50 + 1 || i == counter - 160) {
			r += 127;
		  }

		  // Calculate uint8_t values
		  if(r>127)
		  {
			  r = 127;
		  }
		  if(r<0)
		  {
			  r = 0;
		  }
		  if(g>127)g = 127;
		  if(g<0)
		  {
			  g = 0;
		  }
		  // Set color to pixel buffer
		  strip_buffer.strip[i].fields.red = (uint8_t)r;
		  strip_buffer.strip[i].fields.green = (uint8_t)g;
		  strip_buffer.strip[i].fields.blue = (errCode & ERR_CODE_CAN) == ERR_CODE_CAN ? 0 : (uint8_t)g;
		  strip_buffer.strip[i].fields.brightness =  (uint8_t)0xF;

		}
		// Reset counter
		if (counter == 380) {
		  counter = 0;
		}
		else {
		  counter++;
		}
		lastAPATick = HAL_GetTick();
	}

}

void toggleLED(void)
{
	if(HAL_GetTick() - lastLEDTick >= 250) // status led handling
	{
		lastLEDTick = HAL_GetTick();
		switch(slcan_getState())
		{
		case STATE_OPEN:
			HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
			break;
		case STATE_LISTEN:
			HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
			break;
		default:
			HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
			break;
		}
	}
}

void hUCCB_HandleBufferError()
{
	HAL_UART_Receive_DMA(&huart2, uart_buffers[activeBuffer].data, UART_RX_BUFFER_SIZE);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	uint32_t err = huart->ErrorCode;
	UNUSED(err);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef * huart) //new command received on UART
{
	rxFullFlag = 1; //indicate that data is ready to be parsed
	slCanProccesInputUART((const char*)&uart_buffers[activeBuffer].data); //decode buffer content
	activeBuffer = !activeBuffer; //swap rx buffers
	HAL_UART_Receive_DMA(&huart2, uart_buffers[activeBuffer].data, UART_RX_BUFFER_SIZE);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) // peripheral error
{
	uint32_t err = huart->ErrorCode;
//	HAL_UART_Transmit(&huart2, sl_frame, sl_frame_len, 100);
//	NVIC_SystemReset();
	HAL_UART_Receive_DMA(huart, uart_buffers[activeBuffer].data, UART_RX_BUFFER_SIZE); //ignore and continue to receive data
	UNUSED(err);
}

void HAL_CAN_TxCpltCallback(CAN_HandleTypeDef* hcan)
{
	msgPending = 0;
}

void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef* hcan) // new CAN frame is present
{
	canRxFlags.flags.fifo1 = 1;
//    HAL_CAN_Receive_IT(hcan,CAN_FIFO0);
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan) //peripheral error
{
	HAL_CAN_Receive_IT(hcan, CAN_FIFO0); //ignore errors and try to receive normally
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	HAL_GPIO_TogglePin(LED_SEL_FR_GPIO_PORT, LED_SEL_FR_Pin); //switch LED bar between front and rear
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
	__NOP();
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
	if(strcmp(file, "CAN") == 0)
	{
		errCode |= ERR_CODE_CAN;
	}
//	while(1)
//	{
//
//	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
