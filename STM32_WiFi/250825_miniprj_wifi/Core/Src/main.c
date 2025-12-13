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

#include "esp.h"

#include "Headers.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */
extern cb_data_t cb_data;

uint8_t rx2char;
extern volatile unsigned char rx2Flag;
extern volatile char rx2Data[50];

char buffer[BUFFER_MAX_COUNT];
char buffer_send[BUFFER_MAX_COUNT] = { 0 };

volatile int tim3Flag1Sec=1;
volatile unsigned int tim3Sec;

E_MODE e_mode = E_MONITORING;

volatile int8_t fan_flag = 0;
volatile uint8_t fan_monitoring_switch_flag = 0;

float voltage = 0.f;
float particle_density_w = 0.f;		// last final value
float particle_density_x = 0.f;		// n-2 value
float particle_density_y = 0.f;		// n-1 value
float particle_density_z = 0.f;		// n value
uint32_t vo_value;
uint32_t adc_value = 0;

SENSOR_DATA sensor_data;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */
void delay_us(uint16_t _us);

void set_dht11_pin_output();
void set_dht11_pin_input();
uint8_t dht11_Read(uint8_t *temp, uint8_t *hum);

void fan_pwm_set_percent(uint8_t _pct);

void esp_event(char * recvBuf);
void print_log(char * _msg);
void print_handle_error(char * _msg);
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
	int ret = 0;
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
  MX_TIM3_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
	HAL_TIM_Base_Start(&htim1);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

	print_log("Start main() = wifi\r\n");
	ret |= drv_uart_init();
	ret |= drv_esp_init();
	if (ret != 0)
		print_handle_error("Esp response error\r\n");

	AiotClient_Init();
	if (HAL_TIM_Base_Start_IT(&htim3) != HAL_OK)
		print_handle_error(NULL);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
		if (strstr((char*) cb_data.buf, "+IPD")
				&& cb_data.buf[cb_data.length - 1] == '\n') {
			//?��?��?���??  \r\n+IPD,15:[KSH_LIN]HELLO\n
			strcpy(buffer, strchr((char*) cb_data.buf, '['));
			memset(cb_data.buf, 0x0, sizeof(cb_data.buf));
			cb_data.length = 0;
			esp_event(buffer);
		}
		if (rx2Flag) {
			printf("recv2 : %s\r\n", rx2Data);
			rx2Flag = 0;
		}

		if (tim3Flag1Sec)	//1초에 한번
		{
			tim3Flag1Sec = 0;
			if (!(tim3Sec % 10)) //10초에 한번
			{
				if (esp_get_status() != 0) {
					printf("server connecting ...\r\n");
					esp_client_conn();
				}
			}
			//			printf("tim3Sec : %d\r\n",tim3Sec);
			//			sprintf(sendBuf,"[%s]%s@%s\n","0","TESTCMD","TESTARG");
			//			esp_send_data(sendBuf);

			if (e_mode == E_MANUAL && fan_monitoring_switch_flag > 0)
			{
				fan_monitoring_switch_flag++;

				if (fan_monitoring_switch_flag > 60)
				{
					e_mode = E_MONITORING;
					fan_monitoring_switch_flag = 0;
				}
			}

			if (tim3Sec % 5 == 4)
			{
				// gp2y : ptcl
				HAL_GPIO_WritePin(GPIOA, PTCL_IRLED_Pin, GPIO_PIN_RESET);
				delay_us(280);

				HAL_ADC_Start(&hadc1);
				HAL_ADC_PollForConversion(&hadc1, 100);
				adc_value = HAL_ADC_GetValue(&hadc1);
				HAL_ADC_Stop(&hadc1);

				//delay_us(40);
				HAL_GPIO_WritePin(GPIOA, PTCL_IRLED_Pin, GPIO_PIN_SET);
				//delay_us(9680);

				voltage = adc_value * 3.3 / 4095.0;
				//float offset = 0.3f * (3.3f / 5.0f);
				//float scale = 0.005f * (3.3f / 5.0f);
				//particle_density = (voltage - offset) / scale;
				particle_density_z = (voltage - 0.198) / 0.0033;
				if(particle_density_z < 0)	particle_density_z = 0;

				// dht11 : temp, humi
				uint8_t temp, humi;
				uint8_t result = dht11_Read(&temp, &humi);

				// sensor data
				if (particle_density_y < 0.0001f) particle_density_y = particle_density_z;
				float temp_particle_density = median3_float(particle_density_x, particle_density_y, particle_density_z);

				sensor_data.ptcl = temp_particle_density * 0.65f + particle_density_w * 0.35f;
				particle_density_w = sensor_data.ptcl;
				particle_density_x = particle_density_y;
				particle_density_y = particle_density_z;
				sensor_data.temp = (float)temp;
				sensor_data.humi = (float)humi;

				char msg[50];
				if (result != 0 || temp == 0 || humi == 0)
					;//sprintf(msg, "DHT Error: %d\r\n", result);
				else
				{
					//sprintf(msg, "Ptcl: %.1f Temp: %.1f Humi: %.1f\r\n", (sensor_data.ptcl), (sensor_data.temp), (sensor_data.humi));

					sprintf(buffer_send, "[SENSOR:"SQL_CLN_NAME"]%.1f|%.1f|%.1f\r\n", (sensor_data.ptcl), (sensor_data.temp), (sensor_data.humi));
					esp_send_data(buffer_send);
					print_log(buffer_send);

					// ...
					if (e_mode == E_MONITORING)
					{
						if (fan_flag < 1 && sensor_data.ptcl > 115.f)
						{
							fan_flag = 1;
							fan_pwm_set_percent(100);

							sprintf(msg, "Set Fan Speed: 100\r\n");
							print_log(msg);
						}
						else if (fan_flag > -1 && sensor_data.ptcl < 85.f)
						{
							fan_flag = -1;
							fan_pwm_set_percent(0);

							sprintf(msg, "Set Fan Speed: 0\r\n");
							print_log(msg);
						}

						if (fan_flag == 1 && sensor_data.ptcl < 100.f)
						{
							fan_flag = 0;
							fan_pwm_set_percent(50);

							sprintf(msg, "Set Fan Speed: 50\r\n");
							print_log(msg);
						}
						else if (fan_flag == -1 && sensor_data.ptcl > 100.f)
						{
							fan_flag = 0;
							fan_pwm_set_percent(50);

							sprintf(msg, "Set Fan Speed: 50\r\n");
							print_log(msg);
						}

						/*if (fan_flag == 0)
						{
							fan_pwm_set_percent(50);

							sprintf(msg, "Set Fan Speed: 50\r\n");
							print_log(msg);
						}
						else if (fan_flag == 1)
						{
							fan_pwm_set_percent(100);

							sprintf(msg, "Set Fan Speed: 100\r\n");
							print_log(msg);
						}
						else if (fan_flag == -1)
						{
							fan_pwm_set_percent(0);

							sprintf(msg, "Set Fan Speed: 0\r\n");
							print_log(msg);
						}*/
					}
				}

				//print_log(msg);
			}
		}
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
  hadc1.Init.ScanConvMode = ENABLE;
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
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  htim1.Init.Prescaler = 84 - 1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
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
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 84 - 1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 100 - 1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 60 - 1;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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
  huart6.Init.BaudRate = 38400;
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
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|PTCL_IRLED_Pin|TempHumi_D2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin TempHumi_D2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin|TempHumi_D2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PTCL_IRLED_Pin */
  GPIO_InitStruct.Pin = PTCL_IRLED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(PTCL_IRLED_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void delay_us(uint16_t _us)
{
	__HAL_TIM_SET_COUNTER(&htim1, 0);            // set TIM1 to zero
	while (__HAL_TIM_GET_COUNTER(&htim1) < _us);  // wait for (_us) micro seconds
}

void set_dht11_pin_output()
{
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	GPIO_InitStruct.Pin = TempHumi_D2_Pin;        // DHT 센서가 연결된 핀 지정
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;   // 출력 오픈드레인 모드 설정
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;  // 저속으로 설정 (전송속도 낮음)
	HAL_GPIO_Init(TempHumi_D2_GPIO_Port, &GPIO_InitStruct);
}
void set_dht11_pin_input()
{
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	GPIO_InitStruct.Pin = TempHumi_D2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;       // 입력 모드 설정
	GPIO_InitStruct.Pull = GPIO_NOPULL;           // 내부 풀업이나 풀다운 저항 비활성화
	HAL_GPIO_Init(TempHumi_D2_GPIO_Port, &GPIO_InitStruct);
}
uint8_t dht11_Read(uint8_t *temp, uint8_t *hum) {
    uint8_t bits[5] = {0};
    uint32_t time;

    // MCU -> 센서 신호 송출
    set_dht11_pin_output();
    HAL_GPIO_WritePin(TempHumi_D2_GPIO_Port, TempHumi_D2_Pin, GPIO_PIN_RESET);
    HAL_Delay(20);  // 18~20ms
    HAL_GPIO_WritePin(TempHumi_D2_GPIO_Port, TempHumi_D2_Pin, GPIO_PIN_SET);
    delay_us(40);   // 20~40us

    set_dht11_pin_input();

    // 센서 -> MCU 응답 대기
    time = 0;
    while (HAL_GPIO_ReadPin(TempHumi_D2_GPIO_Port, TempHumi_D2_Pin) == GPIO_PIN_SET) {
        if (++time > 200) return 1; // Timeout
        delay_us(1);
    }

    // LOW(80us) + HIGH(80us)
    time = 0;
    while (HAL_GPIO_ReadPin(TempHumi_D2_GPIO_Port, TempHumi_D2_Pin) == GPIO_PIN_RESET) {
        if (++time > 200) return 2;
        delay_us(1);
    }

    time = 0;
    while (HAL_GPIO_ReadPin(TempHumi_D2_GPIO_Port, TempHumi_D2_Pin) == GPIO_PIN_SET) {
        if (++time > 200) return 3;
        delay_us(1);
    }

    // 40bit 데이터 수신
    for (int i = 0; i < 40; i++) {
        // 1. LOW 시작 비트 감지(50us), LOW가 끝날때까지 대기
        while (HAL_GPIO_ReadPin(TempHumi_D2_GPIO_Port, TempHumi_D2_Pin) == GPIO_PIN_RESET);

        // 2. 중간지점 샘플링 (HIGH: 0이면 약 26~28us, 1이면 70us)
        delay_us(40);
        if (HAL_GPIO_ReadPin(TempHumi_D2_GPIO_Port, TempHumi_D2_Pin)) {
            bits[i / 8] |= (1 << (7 - (i % 8)));
        }
				// 3. HIGH 끝날 때까지 대기
        while (HAL_GPIO_ReadPin(TempHumi_D2_GPIO_Port, TempHumi_D2_Pin) == GPIO_PIN_SET);
    }

    // 체크섬 확인
    uint8_t sum = bits[0] + bits[1] + bits[2] + bits[3];
    if (sum != bits[4]) return 4;

    *hum = bits[0];
    *temp = bits[2];
    return 0; // 성공
}

void fan_pwm_set_percent(uint8_t _pct)
{
	if (_pct > 100)
		_pct = 100; // 최대 100
	__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, _pct); // pwm값 설정 변경
//	if (_pct > 100)
//		_pct = 100;
//	uint32_t period = __HAL_TIM_GET_AUTORELOAD(&htim3);       // ARR
//	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, (period + 1) * _pct / 100);
}

void esp_event(char * recvBuf)
{
	int i = 0;
	char * pToken;
	char * pCommand[COMMAND_MAX_COUNT] = { 0 };

	char buffer_log[BUFFER_MAX_COUNT] = { 0 };

	buffer[strlen(recvBuf) - 1] = '\0';
	sprintf(buffer_log, "\r\nDebug recv : %s\r\n", recvBuf);
	print_log(buffer_log);

	pToken = strtok(recvBuf, "[:|]");
	while (NULL != pToken)
	{
		pCommand[i] = pToken;
		if (++i >= COMMAND_MAX_COUNT)
			break;
		pToken = strtok(NULL, "[:|]");
	}

	//if (!strcmp(pCommand[0], "LOGIN"))
	if (!strcmp(pCommand[0], "SENSOR"))
	{
		strcpy(buffer_send, recvBuf);
		//sprintf(buffer_send, )
	}
	else if (!strcmp(pCommand[0], "LED"))
	{
		strcpy(buffer_send, recvBuf);
		//sprintf(buffer_send, )
	}
	// [FAN:<from>]80
	else if (!strcmp(pCommand[0], "FAN"))
	{
		e_mode = E_MANUAL;

		fan_monitoring_switch_flag = 1;

		uint16_t pwm_value = atoi(pCommand[2]);
		fan_pwm_set_percent(pwm_value);

		sprintf(buffer_send, "[FAN_OK:%s]\r\n", pCommand[1]);
	}
	else
		return;

	esp_send_data(buffer_send);
	sprintf(buffer_log, "Debug send : %s\r\n", buffer_send);
	//snprintf(buffer_log, sizeof(buffer_log), "Debug send : %s\r\n", buffer_send);
	print_log(buffer_log);
}
void print_log(char * _msg)
{
#if DEBUG == 1
	if (NULL != _msg)
		printf(_msg);
#endif
}
void print_handle_error(char * _msg)
{
#if DEBUG == 1
	if (NULL != _msg)
		printf(_msg);
#endif
	Error_Handler();
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
	}
}

PUTCHAR_PROTOTYPE {
	/* Place your implementation of fputc here */
	/* e.g. write a character to the USART6 and Loop until the end of transmission */
	if (HAL_UART_Transmit(&huart2, (uint8_t*) &ch, 1, 10) == HAL_OK)
		return ch;
	return -1;
//		HAL_UART_Transmit(&huart2, (uint8_t*) &ch, 1, 0xFFFF);
//
//	return ch;
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
