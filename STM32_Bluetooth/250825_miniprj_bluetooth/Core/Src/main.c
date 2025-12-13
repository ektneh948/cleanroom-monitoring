/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lcd_i2c.h"
#include "joystick.h"

#include "Headers.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define JOY_X_CHANNEL ADC_CHANNEL_0
#define JOY_Y_CHANNEL ADC_CHANNEL_1
#define SW_PIN   GPIO_PIN_10
#define SW_PORT  GPIOA
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */
Joystick_HandleTypeDef hjs;
Joystick_Tracker_t tracker = { 0 };
volatile int joystick_flag = 0;

int8_t	g_input_x;
int8_t	g_input_y;
uint8_t	g_input_sw;

int8_t cr_cur_idx = 2;		// 0 ~ 3
int8_t cr_max_cnt = 4;		// 4

E_MODE e_mode = E_MONITORING;

uint8_t target_fan_speed = 70;

uint8_t rx2char;
volatile unsigned char rx2Flag = 0;
volatile char rx2Data[50];
volatile unsigned char btFlag = 0;
uint8_t btchar;
char btData[50];
volatile int tim3Flag1Sec=1;
volatile unsigned int tim3Sec;

SENSOR_DATA sensor_data;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM3_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */
void input_joystick();
void bluetooth_Event();
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_USART6_UART_Init();
  MX_I2C1_Init();
  MX_TIM3_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(1000);
  lcd_init(&hi2c1);
  Joystick_Init(&hjs, &hadc1, JOY_X_CHANNEL, &hadc1, JOY_Y_CHANNEL, SW_PORT, SW_PIN);

  HAL_TIM_Base_Start_IT(&htim3);

	//HAL_UART_Receive_IT(&huart2, &rx2char, 1);
	HAL_UART_Receive_IT(&huart6, &btchar, 1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {

		if (rx2Flag) {
			printf("recv2 : %s\r\n", rx2Data);
			rx2Flag = 0;

			//  HAL_UART_Transmit(&huart6, (uint8_t *)buf, strlen(buf), 0xFFFF);
		}
		if (btFlag) {
			printf("bt : %s\r\n", btData);
			btFlag = 0;
			bluetooth_Event();

		}

		//	  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
		//	  HAL_Delay(500);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		input_joystick();

		if (joystick_flag)
		{
			//printf("x : %d / y : %d / sw : %d\r\n", g_input_x, g_input_y, g_input_sw);

			joystick_flag = 0;

			// ...
			if (g_input_sw > 0)
			{
				if (e_mode == E_MONITORING)
				{
					e_mode = E_MANUAL;

					target_fan_speed = 70;
					g_input_sw = 0;
				}
			}

			// ...
			if (g_input_x > 0)
			{
				cr_cur_idx = (cr_cur_idx + 1) % cr_max_cnt;

				if (e_mode == E_MONITORING) {
					char sendBuf[BUFFER_MAX_COUNT] = { 0 };
					sprintf(sendBuf, "[GETSENSOR:CMS_SQL]STM_WF%d\n", cr_cur_idx + 1);
					HAL_UART_Transmit(&huart6, (uint8_t*) sendBuf, strlen(sendBuf), 0xFFFF);
				}
			} else if (g_input_x < 0) {
				cr_cur_idx = (cr_cur_idx - 1 < 0 ? cr_max_cnt - 1 : cr_cur_idx - 1);

				if (e_mode == E_MONITORING) {
					char sendBuf[BUFFER_MAX_COUNT] = { 0 };
					sprintf(sendBuf, "[GETSENSOR:CMS_SQL]STM_WF%d\n", cr_cur_idx + 1);
					HAL_UART_Transmit(&huart6, (uint8_t*) sendBuf, strlen(sendBuf), 0xFFFF);
				}
			}

			// ...
			if (e_mode == E_MANUAL)
			{
				if (g_input_y > 0)
					target_fan_speed = min(target_fan_speed + 5, 100);
				else if (g_input_y < 0)
					target_fan_speed = max(target_fan_speed - 5, 0);

				char buf1[17];
				char buf2[17];

				lcd_clear();
				lcd_set_cursor(0, 0);
				sprintf(buf1, "CR : #%d", cr_cur_idx + 1);
				lcd_send_string(buf1);

				lcd_set_cursor(1, 0);
				sprintf(buf2, "FAN SPD : %d", target_fan_speed);
				lcd_send_string(buf2);

				if (g_input_sw > 0)
				{
					e_mode = E_MONITORING;

					char sendBuf[BUFFER_MAX_COUNT] = { 0 };
					sprintf(sendBuf, "[FAN:CMS_SQL]%d|STM_WF%d\n", target_fan_speed, cr_cur_idx + 1);
					HAL_UART_Transmit(&huart6, (uint8_t*) sendBuf, strlen(sendBuf), 0xFFFF);
				}
			}

			g_input_x = 0;
			g_input_y = 0;
			g_input_sw = 0;
		}

		if (tim3Flag1Sec)	//1초에 한번
		{
			tim3Flag1Sec = 0;

			if (!(tim3Sec % 3) && e_mode == E_MONITORING) {
				char sendBuf[BUFFER_MAX_COUNT] = { 0 };
				sprintf(sendBuf, "[GETSENSOR:CMS_SQL]STM_WF%d\n", cr_cur_idx + 1);
				HAL_UART_Transmit(&huart6, (uint8_t*) sendBuf, strlen(sendBuf), 0xFFFF);

				//lcd_set_cursor(1, 0);
				//lcd_send_string("CR:");
			}

			HAL_GPIO_WritePin(GPIOA, RGB_R_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOB, RGB_G_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOB, RGB_B_Pin, GPIO_PIN_RESET);
			if (sensor_data.ptcl < 100.f)
				HAL_GPIO_WritePin(GPIOB, RGB_G_Pin, GPIO_PIN_SET);
			else
				HAL_GPIO_WritePin(GPIOA, RGB_R_Pin, GPIO_PIN_SET);
		}
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 84 - 1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 1000 - 1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

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
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 9600;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|RGB_R_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, RGB_G_Pin|RGB_B_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin RGB_R_Pin */
  GPIO_InitStruct.Pin = LD2_Pin|RGB_R_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : RGB_G_Pin RGB_B_Pin */
  GPIO_InitStruct.Pin = RGB_G_Pin|RGB_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PA10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void MX_GPIO_LED_ON(int pin)
{
	HAL_GPIO_WritePin(LD2_GPIO_Port, pin, GPIO_PIN_SET);
}
void MX_GPIO_LED_OFF(int pin)
{
	HAL_GPIO_WritePin(LD2_GPIO_Port, pin, GPIO_PIN_RESET);
}

void input_joystick()
{
	Joystick_Data_t data = Joystick_Read(&hjs, 10);
	Joystick_Track(&tracker, &data, 3000, 1000);

	if (g_input_x == 0) {
		if ((tracker.x_plus_history & 0x03) == 0x01)
			g_input_x = 1;
		else if ((tracker.x_minus_history & 0x03) == 0x01)
			g_input_x = -1;
	}
	if (g_input_y == 0) {
		if ((tracker.y_plus_history & 0x01) == 0x01)
			g_input_y = -1;
		else if ((tracker.y_minus_history & 0x01 ) == 0x01)
			g_input_y = 1;
	}

	if (g_input_sw == 0) g_input_sw = (data.button == GPIO_PIN_SET);
}

void bluetooth_Event()
{
    int i = 0;
    char *pToken;
    char *pArray[COMMAND_MAX_COUNT] = {0};
    char recvBuf[BUFFER_MAX_COUNT] = {0};
    //char sendBuf[BUFFER_MAX_COUNT] = {0};

    strcpy(recvBuf, btData);
    printf("btData : %s\r\n", btData);


    pToken = strtok(recvBuf, "[:|]");
    while(pToken != NULL)
    {
        pArray[i] = pToken;
        if(++i >= COMMAND_MAX_COUNT)
            break;
        pToken = strtok(NULL, "[:|]");
    }

//    sprintf(sendBuf,"[GETSENSOR:CMS_SQL]\r\n");
//    HAL_UART_Transmit(&huart6, (uint8_t *)sendBuf, strlen(sendBuf), 0xFFFF);

    if (!strcmp(pArray[0], "GETSENSOR") && e_mode == E_MONITORING)
    {
		char buf1[17];
		char buf2[17];
		lcd_clear();

		lcd_set_cursor(0, 0);
		sprintf(buf1, "CR : #%d", cr_cur_idx + 1);
		lcd_send_string(buf1);

		lcd_set_cursor(1, 0);
		//P:000 T:00 H:00
		sensor_data.ptcl = atof(pArray[2]);
		sensor_data.temp = atof(pArray[3]);
		sensor_data.humi = atof(pArray[4]);
		sprintf(buf2, "P:%3d T:%s H:%s", (int) (sensor_data.ptcl), pArray[3],
				pArray[4]);
		lcd_send_string(buf2);
    }

	//조이스틱 양 옆으로 움직이면 룸 번호 바뀌고, 버튼 누르면 해당 룸 번호의 데이터 가져오게
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)		//1ms 마다 호출
{
	if (htim->Instance == TIM3) {
		static int tim3Cnt = 0;
		tim3Cnt++;
		if (tim3Cnt >= 1000) //1ms * 1000 = 1Sec
		{
			tim3Flag1Sec = 1;
			tim3Sec++;
			tim3Cnt = 0;
		}

		static int joystickCnt = 0;
		joystickCnt++;
		if (joystickCnt >= 500)
		{
			joystick_flag = 1;
			joystickCnt = 0;
		}
	}
}

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART6 and Loop until the end of transmission */
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);

  return ch;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

   if(huart->Instance == USART2)
    {
    	static int i=0;
    	rx2Data[i] = rx2char;
    	if((rx2Data[i] == '\r')||(rx2Data[i] == '\n'))
    	{
    		rx2Data[i] = '\0';
    		rx2Flag = 1;
    		i = 0;
    	}
    	else
    	{
    		i++;
    	}
    	HAL_UART_Receive_IT(&huart2, &rx2char,1);
    }
    //블루투스
    if(huart->Instance == USART6)
        {
            static int i = 0;
            btData[i] = btchar;
            if((btData[i] == '\n') || (btData[i] == '\r'))
            {
                btData[i] = '\0';
                btFlag = 1;
                i = 0;

                printf("BT: Data received complete\r\n");
            }
            else
            {
                i++;
                if(i >= sizeof(btData) - 1)
                {
                    printf("BT: RX buffer overflow, resetting\r\n");
                    i = 0;
                    btData[0] = '\0';
                }
            }
            HAL_UART_Receive_IT(&huart6, &btchar, 1);
        }
}

uint16_t Read_ADC_Channel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    uint16_t val = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return val;
}

/* USER CODE END 4 */

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
#ifdef USE_FULL_ASSERT
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
