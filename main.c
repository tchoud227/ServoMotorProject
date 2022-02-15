/* USER CODE BEGIN Header */
/**
  ******************************************************************************
Tanim Choudhury
CPE 3000
Servo Memory Game

A memory game where a cpu generates a 5 numbered sequence.
The user will then memorize the sequence and input the sequence.
if the result matches the generated sequence the cpu will admit defeat.
Otherwise, the user will have 2 more chances to get it right before losing.
								*Note*
Make sure to adjust the max and min in Private define to pulse widths for your particular servo.
If the for loop in lines 107 gives angle positions that are hard to distinguish.
You can hardcode the proper pulse widths instead. Lines 115-119 has a template set up.
Make sure to uncomment and comment the prior for loop(ctrl / is a shortcut for commenting)

  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stdlib.h"
#include "stdbool.h"
#include "time.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MY_SERVO_MAX	76 //start position
#define MY_SERVO_MIN	17 //your turn position
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//game_elements
struct servo_variables{
	uint16_t start,yourturn;
	uint8_t number_pwm[5];
	uint8_t cpu_sequence[5];
	uint8_t user_sequence[5];

}my_servo;
//constant game_state
enum game_state_e{
	STATE_CPU_TURN,
	STATE_USER_TURN,
	STATE_RESULT,
}game_state;

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	uint8_t input_second_counter = 0; //used to keep track when over 5 input
	uint8_t input_sequence_counter = 0;// used to keep track when 5 sequence is reached
	uint8_t user_chance_counter = 0; // used to keep track of number of chances left
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  //SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
	//initialize servo motor
	my_servo.start = MY_SERVO_MAX; //starting position is 180 degrees
	my_servo.yourturn = MY_SERVO_MIN; // your turn is at 0 degrees
	for(int i = 0; i < 5; i++){
		//calculate individual servo pwm positions from 1-5
		//pulse width = max width + ((pulse width +1)(range of width)/positions +1
		my_servo.number_pwm[i] = my_servo.start + ((i+1)*((my_servo.yourturn - my_servo.start)/6));
	}
	/*	if the above for loop is giving weird angle positions you can hardcode ccr values into the 1-5 positons my_servo.number_pwm[i]
		my_servo.number_pwm[0] =
		my_servo.number_pwm[1] =
		my_servo.number_pwm[2] =
		my_servo.number_pwm[3] =
		my_servo.number_pwm[4] =
	*/
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1); //start pwm

	htim2.Instance -> CCR1 = my_servo.start; //set to start position
	game_state = STATE_CPU_TURN;			//CPU starts since it needs to provide random sequence
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

	while (1)
	{

/* USER CODE END WHILE */
		//this infinite loop switches between 3 games states depending on the game_state value
		switch(game_state){
		case STATE_CPU_TURN:
			htim2.Instance -> CCR1 = my_servo.start; //set to start position
			HAL_Delay(2000);
			//Calculate 5 random sequences without repetition
			srand(HAL_GetTick()); //initialized to a random runtime value
			my_servo.cpu_sequence[0] = (rand()%5)+1;
			for(int i = 1; i < 5; i++){
				do{
					my_servo.cpu_sequence[i] = (rand()%5)+1; //sets a number 1-5 in the current index of the sequence
				}
				while(my_servo.cpu_sequence[i-1] == my_servo.cpu_sequence[i]); //doesn't iterate if adjacent repeating value
			}
			//Move servo motor according the random sequence positions
			for(int i = 0; i < 5; i++){
				htim2.Instance -> CCR1 = my_servo.number_pwm[my_servo.cpu_sequence[i]-1];
				HAL_Delay(1000);
			}
			//Move servo motor to Your Turn position
			htim2.Instance -> CCR1 = my_servo.yourturn;
			//reset counters
			input_second_counter = 0;
			input_sequence_counter = 0;
			user_chance_counter = 0;
			game_state = STATE_USER_TURN;
			break;

		case STATE_USER_TURN:
		{
			uint32_t saved_time = HAL_GetTick(); //starts a timer in a way
			uint8_t blink_count = 0;
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET); //led initially off
			//measure the time, user has press and hold the button
			while(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET){
				if(HAL_GetTick() - saved_time > 500){ //if current runtime minus the comparing runtime is greater than half a second
					saved_time = HAL_GetTick(); //new comparing runtime
					HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
					blink_count++;
					if (blink_count == 2){ //consider after 1 second
						input_second_counter++;
						blink_count = 0;
						//blink led at 10hz for 2 seconds if pressed more than 5 seconds.
						if(input_second_counter>5){
							for(int i = 0; i<40;i++){//40*50/1000 = 2sec
								HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
								HAL_Delay(50);
							}
							saved_time = HAL_GetTick();//new comparing runtime
							input_second_counter=0;//reset over 5 sequence counter
						}
					}
				}
			}
			//check if valid button was pressed then move servo to that position
			if(input_second_counter>0){
				htim2.Instance -> CCR1 = my_servo.number_pwm[input_second_counter-1];//actual number
				my_servo.user_sequence[input_sequence_counter] = input_second_counter;//current sequence index
				input_sequence_counter++;//next sequence index
				input_second_counter = 0; // reset second counter
				//when all five inputs has given by the user go to result state.
				if(input_sequence_counter>=5){
					input_sequence_counter=0;
					game_state = STATE_RESULT;
				}
			}
			break;
		}
		case STATE_RESULT:
		{
			bool result_matched = true;
			//checking the result
			for(int i = 0; i<5;i++){
				if(my_servo.cpu_sequence[i] != my_servo.user_sequence[i]){
					result_matched = false;
				}
			}
			//Accepting the defeat, slow return to start position
			if(result_matched == true){
				while(htim2.Instance -> CCR1 < my_servo.start){
					htim2.Instance -> CCR1++;
					HAL_Delay(50);
					game_state = STATE_CPU_TURN;
				}
			}else{  //if user failed
				//blink led for 3 seconds at 10hz
				for(int i = 0; i<60;i++){ //60 * 50/1000 =3sec
					HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
					HAL_Delay(50);
				}
				htim2.Instance -> CCR1 = my_servo.yourturn;
				HAL_Delay(200);
				user_chance_counter++;
				//give user another chance
				if(user_chance_counter<3){
					game_state = STATE_USER_TURN;
				}else{
					//coming back to start slowly
					user_chance_counter=0; //reset chances
					while(htim2.Instance -> CCR1 < my_servo.start){
						htim2.Instance -> CCR1++;
						HAL_Delay(50);
						game_state = STATE_CPU_TURN;
					}
				}

			}
			break;
		}
		}

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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
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
  htim2.Init.Prescaler = 128-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1250-1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
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
  sConfigOC.Pulse = 150;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : USART_TX_Pin USART_RX_Pin */
  GPIO_InitStruct.Pin = USART_TX_Pin|USART_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */

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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
