/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "a.ModbusRTUTask.h"
#include "b.IOManagerTask.h"
#include "c.TCPProtocolTask.h"
#include "d.HTTPProtocolTask.h"
#include "e.ATcommandTask.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart5;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;
DMA_HandleTypeDef hdma_usart3_rx;
DMA_HandleTypeDef hdma_usart3_tx;

osThreadId MODBUS_RTU_TASKHandle;
osThreadId IO_MANAGER_TASKHandle;
osThreadId TCP_PROTO_TASKHandle;
osThreadId HTTP_PROTO_TASKHandle;
osThreadId ATCOMMAND_TASKHandle;
//osMessageQId myQueue01Handle;
/* USER CODE BEGIN PV */
osMailQId IOTaskReceive_MQ;                   // Mail Queue ( IO Manager Queue )
//TASK IO
osMailQId IOTaskTransferToRTU_MQ;       // Mail Queue Notify Change
osMailQId IOTaskTransferToTCP_MQ;       // Mail Queue Notify Change
osMailQId IOTaskTransferToHTTP_MQ;    // Mail Queue Notify Change

//DECLARATION
uint8_t RX2_Buffer[RX2_SIZE] = {0};
volatile uint16_t RX2_ActualSize = 0;
uint8_t TX2_Buffer[TX2_SIZE] = {0};

uint8_t RX3_Buffer[RX3_SIZE] = {0};
volatile uint16_t RX3_ActualSize = 0;

uint8_t RX5_Buffer[RX5_SIZE] = {0};
uint8_t TX5_Buffer[TX5_SIZE] = {0};
volatile uint16_t RX5_ActualSize = 0;
volatile uint8_t RX5_Ready = 0;

extern   uint8_t Input_MainDatabase; // from #include "b.IOManagerTask.h"
uint8_t Input_MainDatabase_Pre;

extern SystemConfig_t g_config; // flask from TCPProtocolTask

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
static void MX_UART5_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
void ModbusRTUTask(void const * argument);
void IOManagerTask(void const * argument);
void TCPProtocolTask(void const * argument);
void HTTPProtocolTask(void const * argument);
void ATcommandTask(void const * argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//2 HÀM ĐỂ DEBUG RUN TIME TRONG Window + Show View + Free RTOS + Free RTOS Task List
//(Lưu ý: Chạy Chương trình xong - tạm dừng chương trình khi debug mới xem được)
/* Hàm này được RTOS gọi một lần khi bắt đầu chạy Scheduler */
void configureTimerForRunTimeStats(void) {}
unsigned long getRunTimeCounterValue(void) {
    return HAL_GetTick(); /* Hàm này được RTOS gọi liên tục để lấy giá trị thời gian (độ phân giải 1ms) */
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) { // Function : Read Input 10ms
	if (htim->Instance == TIM1)
	{
		uint8_t tmp = 0;
		for (int i = 0; i < 8; i++)
		{
			tmp |= (((GPIO_INPUT[i].port->IDR & GPIO_INPUT[i].pin) != 0) << i);
		}

		tmp ^= 0xFF;   // <<< ĐẢO BIT

		if (tmp != Input_MainDatabase_Pre)
		{
			Input_MainDatabase = tmp;
			Input_MainDatabase_Pre = tmp;
			osSignalSet(IO_MANAGER_TASKHandle, SIG_SCAN_INPUT);
		}
	}
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size){ //===UART2 + UART3 IDLE DMA====//
	if(huart->Instance == USART2)
	{
		RX2_ActualSize = Size;
		osSignalSet(TCP_PROTO_TASKHandle, SIG_DMA_UART2); // Authorize run TCP Task

		HAL_UARTEx_ReceiveToIdle_DMA(&huart2, RX2_Buffer, RX2_SIZE);
		__HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);

	}
	if(huart->Instance == USART3)
	{
		RX3_ActualSize = Size;
		osSignalSet(HTTP_PROTO_TASKHandle, SIG_DMA_UART3);// Authorize run HTTP Task

		HAL_UARTEx_ReceiveToIdle_DMA(&huart3, RX3_Buffer, RX3_SIZE);
		__HAL_DMA_DISABLE_IT(huart3.hdmarx, DMA_IT_HT);
	}
	if(huart -> Instance == UART5){
		RX5_ActualSize = Size;
		osSignalSet(MODBUS_RTU_TASKHandle, SIG_IT_UART5);
		HAL_UARTEx_ReceiveToIdle_IT(&huart5, RX5_Buffer, RX5_SIZE);
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){ //=========EXT INTERRUPT=============//
	if(GPIO_Pin == BT1_Pin){
		osSignalSet(IO_MANAGER_TASKHandle, SIG_BT1);
	}
	else if(GPIO_Pin == BT2_Pin){
		osSignalSet(IO_MANAGER_TASKHandle, SIG_BT2);
	}
	else if(GPIO_Pin == BT3_Pin){
		osSignalSet(IO_MANAGER_TASKHandle, SIG_BT3);
	}
	else if(GPIO_Pin == BT4_Pin){
		osSignalSet(IO_MANAGER_TASKHandle, SIG_BT4);
	}
	else if(GPIO_Pin == BT_RST_WIFI_Pin){
		if(ATCOMMAND_TASKHandle != NULL) {
			osSignalSet(ATCOMMAND_TASKHandle, SIG_RST_WIFI);
		}
	}
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  //HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  Flash_Read_Config(&g_config); // Read Flask Memories

  if (g_config.magic == 0xA5A5A5A5)
  {
	  // HTTP
	  Port_HTTP = g_config.Port_HTTP_Flask;
	  St_ID = g_config.St_ID_Flask;
      //strcpy(Token, g_config.Token_Flask);
	  strcpy(Endpoint, g_config.Endpoint_Flask);
	  strcpy(Pass_HTTP, g_config.Pass_HTTP_Flask);
	  strcpy(Event, g_config.Event_Flask);
	  strcpy(Ip_HTTP, g_config.Ip_HTTP_Flask);
	  strcpy(User, g_config.User_Flask);

	  // FMS
		On_FMS   = g_config.On_FMS_Flask;
		strcpy(Tk1, g_config.Tk1_Flask);
		strcpy(Tk2, g_config.Tk2_Flask);
		strcpy(Tk3, g_config.Tk3_Flask);
		strcpy(Tk4, g_config.Tk4_Flask);
		strcpy(Pt1, g_config.Pt1_Flask);
		strcpy(Pt2, g_config.Pt2_Flask);
		strcpy(Pt3, g_config.Pt3_Flask);
		strcpy(Pt4, g_config.Pt4_Flask);

	  // TCP + RTU
	  On_TCP   = g_config.On_TCP_Flask;
	  On_RTU   = g_config.On_RTU_Flask;
	  Slave_ID = g_config.Slave_ID_Flask;
	  Port_TCP = g_config.Port_TCP_Flask;
  }
  else
  {
	  // Default
	  Port_HTTP = 5000;
	  On_TCP   = 1;
	  On_RTU   = 0;
	  Slave_ID = 1;
	  Port_TCP = 8080;
  }
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_UART5_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(500);
  HAL_GPIO_WritePin(RST_WIFI_GPIO_Port, RST_WIFI_Pin, 1);

//  wifi_pulse(1000, 1000);
  Reset_Wifi();

  HAL_TIM_Base_Start_IT(&htim1); // Timer interrupt

  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, RX2_Buffer, RX2_SIZE);
  __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);

  HAL_UARTEx_ReceiveToIdle_DMA(&huart3, RX3_Buffer, RX3_SIZE);
  __HAL_DMA_DISABLE_IT(huart3.hdmarx, DMA_IT_HT);

  HAL_UARTEx_ReceiveToIdle_IT(&huart5, RX5_Buffer, RX5_SIZE);

  //RST_WIFI_GPIO_Port ->  BSRR = RST_WIFI_Pin; // Set Bit 1 to Start HLK-RM08S
  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* definition and creation of myQueue01 */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
   osMailQDef(IOTaskReceive_MQ, 4, IO_State_t);  // Mail Queue
   IOTaskReceive_MQ = osMailCreate(osMailQ(IOTaskReceive_MQ), NULL);  // Mail Queue
   //============IO TASK=========================//
	osMailQDef(IOTaskTransferToRTU_MQ, 4, IO_State_t);  // Mail Queue Notify Change IO
	IOTaskTransferToRTU_MQ = osMailCreate(osMailQ(IOTaskTransferToRTU_MQ), NULL);  // Mail Queue Notify Change IO

	osMailQDef(IOTaskTransferToTCP_MQ, 4, IO_State_t);  // Mail Queue Notify Change IO
	IOTaskTransferToTCP_MQ = osMailCreate(osMailQ(IOTaskTransferToTCP_MQ), NULL);  // Mail Queue Notify Change IO

	osMailQDef(IOTaskTransferToHTTP_MQ, 4, IO_State_t);  // Mail Queue Notify Change IO
	IOTaskTransferToHTTP_MQ = osMailCreate(osMailQ(IOTaskTransferToHTTP_MQ), NULL);  // Mail Queue Notify Change IO
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of MODBUS_RTU_TASK */
  osThreadDef(MODBUS_RTU_TASK, ModbusRTUTask, osPriorityNormal, 0, 128);
  MODBUS_RTU_TASKHandle = osThreadCreate(osThread(MODBUS_RTU_TASK), NULL);

  /* definition and creation of IO_MANAGER_TASK */
  osThreadDef(IO_MANAGER_TASK, IOManagerTask, osPriorityLow, 0, 128);
  IO_MANAGER_TASKHandle = osThreadCreate(osThread(IO_MANAGER_TASK), NULL);

  /* definition and creation of TCP_PROTO_TASK */
  osThreadDef(TCP_PROTO_TASK, TCPProtocolTask, osPriorityNormal, 0, 768);
  TCP_PROTO_TASKHandle = osThreadCreate(osThread(TCP_PROTO_TASK), NULL);

  /* definition and creation of HTTP_PROTO_TASK */
  osThreadDef(HTTP_PROTO_TASK, HTTPProtocolTask, osPriorityNormal, 0, 512);
  HTTP_PROTO_TASKHandle = osThreadCreate(osThread(HTTP_PROTO_TASK), NULL);

  /* definition and creation of ATCOMMAND_TASK */
  osThreadDef(ATCOMMAND_TASK, ATcommandTask, osPriorityHigh, 0, 512);
  ATCOMMAND_TASKHandle = osThreadCreate(osThread(ATCOMMAND_TASK), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.Prediv1Source = RCC_PREDIV1_SOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  RCC_OscInitStruct.PLL2.PLL2State = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the Systick interrupt time
  */
  __HAL_RCC_PLLI2S_ENABLE();
}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 99;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 7999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief UART5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART5_Init(void)
{

  /* USER CODE BEGIN UART5_Init 0 */

  /* USER CODE END UART5_Init 0 */

  /* USER CODE BEGIN UART5_Init 1 */

  /* USER CODE END UART5_Init 1 */
  huart5.Instance = UART5;
  huart5.Init.BaudRate = 115200;
  huart5.Init.WordLength = UART_WORDLENGTH_8B;
  huart5.Init.StopBits = UART_STOPBITS_1;
  huart5.Init.Parity = UART_PARITY_NONE;
  huart5.Init.Mode = UART_MODE_TX_RX;
  huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart5.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART5_Init 2 */

  /* USER CODE END UART5_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
  /* DMA1_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 7, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);
  /* DMA1_Channel7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 7, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RST_WIFI_GPIO_Port, RST_WIFI_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, OUT1_Pin|OUT2_Pin|OUT3_Pin|OUT4_Pin
                          |LED_BT1_Pin|LED_BT2_Pin|LED_BT3_Pin|LED_BT4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, OUT5_Pin|OUT6_Pin|OUT7_Pin|OUT8_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : BT1_Pin BT2_Pin BT3_Pin BT4_Pin */
  GPIO_InitStruct.Pin = BT1_Pin|BT2_Pin|BT3_Pin|BT4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : RST_WIFI_Pin */
  GPIO_InitStruct.Pin = RST_WIFI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RST_WIFI_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : IN1_Pin IN2_Pin IN3_Pin IN4_Pin */
  GPIO_InitStruct.Pin = IN1_Pin|IN2_Pin|IN3_Pin|IN4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : IN5_Pin IN6_Pin */
  GPIO_InitStruct.Pin = IN5_Pin|IN6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : IN7_Pin IN8_Pin */
  GPIO_InitStruct.Pin = IN7_Pin|IN8_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : OUT1_Pin OUT2_Pin OUT3_Pin OUT4_Pin
                           LED_BT1_Pin LED_BT2_Pin LED_BT3_Pin LED_BT4_Pin */
  GPIO_InitStruct.Pin = OUT1_Pin|OUT2_Pin|OUT3_Pin|OUT4_Pin
                          |LED_BT1_Pin|LED_BT2_Pin|LED_BT3_Pin|LED_BT4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : OUT5_Pin OUT6_Pin OUT7_Pin OUT8_Pin */
  GPIO_InitStruct.Pin = OUT5_Pin|OUT6_Pin|OUT7_Pin|OUT8_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : BT_RST_WIFI_Pin */
  GPIO_InitStruct.Pin = BT_RST_WIFI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BT_RST_WIFI_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
	/* Nếu chương trình rơi vào đây, có nghĩa là Task có tên pcTaskName đã bị tràn Stack.
	   Tại đây bạn có thể đặt Breakpoint để debug hoặc cho LED nhấp nháy báo lỗi.
	*/
	( void ) xTask;
	( void ) pcTaskName;

	for( ;; );
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_ModbusRTUTask */
/**
  * @brief  Function implementing the MODBUS_RTU_TASK thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_ModbusRTUTask */
void ModbusRTUTask(void const * argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
	Modbus_RTU_Communication(argument);
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_IOManagerTask */
/**
* @brief Function implementing the IO_MANAGER_TASK thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_IOManagerTask */
void IOManagerTask(void const * argument)
{
  /* USER CODE BEGIN IOManagerTask */
  /* Infinite loop */
	IOManagerTaskFunction(argument);
  /* USER CODE END IOManagerTask */
}

/* USER CODE BEGIN Header_TCPProtocolTask */
/**
* @brief Function implementing the TCP_PROTO_TASK thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_TCPProtocolTask */
void TCPProtocolTask(void const * argument)
{
  /* USER CODE BEGIN TCPProtocolTask */
  /* Infinite loop */
	Parse_JSON_TCP(argument);
  /* USER CODE END TCPProtocolTask */
}

/* USER CODE BEGIN Header_HTTPProtocolTask */
/**
* @brief Function implementing the HTTP_PROTO_TASK thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_HTTPProtocolTask */
void HTTPProtocolTask(void const * argument)
{
  /* USER CODE BEGIN HTTPProtocolTask */
  /* Infinite loop */
	Send_IOStateToServer(argument);
  /* USER CODE END HTTPProtocolTask */
}

/* USER CODE BEGIN Header_ATcommandTask */
/**
* @brief Function implementing the ATCOMMAND_TASK thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_ATcommandTask */
void ATcommandTask(void const * argument)
{
  /* USER CODE BEGIN ATcommandTask */
  /* Infinite loop */
	ATcommand_Function(argument);
  /* USER CODE END ATcommandTask */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
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
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
