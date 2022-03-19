/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Alarm Clock
  * @version		: 1.09
  * @author			: Tim White
  * @date			: 2022-03-19
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
 RTC_HandleTypeDef hrtc;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_RTC_Init(void);
/* USER CODE BEGIN PFP */
void main_menu();
void set_time();
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
	RTC_TimeTypeDef currentTime;  	// Get time structure
	RTC_DateTypeDef currentDate;   	// Get date structure

	unsigned int decimal_numbers[10] = {
			0b00001100, // 0
			0b10001100, // 1
			0b01001100, // 2
			0b11001100, // 3
			0b00101100, // 4
			0b10101100, // 5
			0b01101100, // 6
			0b11101100, // 7
			0b00011100, // 8
			0b10011100  // 9
	};
	unsigned int hacky_hex[60] = {
			0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
			0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,
			0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,
			0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,
			0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,
			0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59
	};

	unsigned int btn_pressed=0;
	unsigned int byte=0;
	unsigned int prev_byte=0;

	int rem=0;
	int digit_count=0;
	int mu=0;
	int num2=0;

	int current_hours=0;
	int current_minutes=0;
	int current_seconds=0;
	int last_seconds=0;

	int menu_text=0;
	int scroll_y_index=0;
	int scroll_x_index=0;

	int hour_digit1=0;
	int hour_digit2=0;
	int minute_digit1=0;
	int minute_digit2=0;

	int alarm_hour=0;
	int alarm_minutes=0;

	int snoozed=0;
	int alarming=0;
	int mode;
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
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */


  /**********************************************************/
  // Display functions:
  //	- Write(char i): Stage write data
  //	- Command(char i): Stage write command
  //	- Set_Position(char i): Goto display location
  //	- Init_Display(): Wake, function set, cursor, clear
  //	- Send(): Send data or command from GPIOA
  //	- Clear_Home(): Clear display and goto home position

  unsigned char reverse(unsigned char x) {
	  // Reverse incoming char b
	  // example: 1100 -> 0011
     x = (x & 0xF0) >> 4 | (x & 0x0F) << 4;
     x = (x & 0xCC) >> 2 | (x & 0x33) << 2;
     x = (x & 0xAA) >> 1 | (x & 0x55) << 1;
     return x;
  }

  void send()
  {
	  // Falling-edge triggered Display control
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 1); // Enable pin
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 0); // Clock enable: falling edge
	  HAL_Delay(5);
  }

  void write(char i)
  {
	  // Set write mode, load char i into GPIOA pins, send.
	  GPIOA->ODR = 0x00; // Clear GPIOA
	  mode=0x400; // Write Data Mode
	  GPIOA->ODR = i | mode;
	  send();
  }

  void command(char i)
  {
	  // Set command mode & write mode, load char i into GPIOA pins, send.
	  mode=0x000; // Write Command Mode
	  GPIOA->ODR = i | mode;
	  send();
  }

  void set_position(char i)
  {
	i = reverse(i);
	command(0b00000001|i);

	/* Driver function to test above function */
  }

  void init_display()
  {
	command(0b00110000); // wake display
	command(0b00110000); // wake display
	command(0b00110000); // wake display
	HAL_Delay(100);
	command(0b00111100); // function set 8-bit, 2-line
	command(0b10000000); // clear Display
	command(0b01000000); // Return home
	write(0b10000010);
  }


  void clear_home(){
	command(0x80); // clear display
	command(0x40); // return home
  }

  /**********************************************************/
  // Conversion functions:
  //	- write_binary_to_decimal(int num):
  //		Lookup function for each decimal digit of num.


  void write_binary_to_decimal(int num){
	if(num==0){
		write(decimal_numbers[0]);
		return;
	}
	rem=0;			// Remainder after division by current multiplier. E.g. 2345{number} / 100{multiplier} = 234{rem}
	digit_count=0; 	// Total decimal digits in num
	mu=0;			// Current multiplier (assuming decimal base 10)
	num2=num;		//	Buffer of num used to find digit_count.

	// Solve for digit_count
	while (num2 > 0)
	{
		digit_count=digit_count+1;
		num2 /= 10;
	}

	// write decimal_number corresponding to each digit, MSB to LSB
	while (digit_count>0)
	{
		// For last digit in num, write that digit
		if(digit_count==1){
			write(decimal_numbers[num]);
		}
		// otherwise, write each digit
		else
		{
			mu = 10;
			for (int i = 1; i < digit_count-1; ++i){ mu *= 10; }
			rem = num / mu;
			num -= rem * mu; // decrement num by MSB;
			write(decimal_numbers[rem]);
		}
		digit_count-=1;
	}
  }

  /**********************************************************/
  // Update functions:
  //	- Process_Inputs()
  //	- Update_Display()
  //	- Update_Time()

  int process_inputs(){

	// For reference - Button IDs: OK=0, BK=1, D=2, L=3, R=4, U=5, NOTHING=6
	byte=0;
	prev_byte=6;
	btn_pressed=0;

	while(1) {
		byte=0;
		btn_pressed=0;

		// Parallel load sequence, trigger serial receive
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  1); // CE
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  1); // PL
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  0); // PL
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  1); // PL
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  0); // CE
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  0); // CP
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  1); // CP

		// Buffer all incoming bits into Byte
		for(int i=0; i<8; i++){
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, 0); // CP
			HAL_Delay(5);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, 1); // CP
			HAL_Delay(5);
			byte = (byte | ((HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15)) << i)); // Store all bits in a buffer
			if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15)==0) {
				btn_pressed=i;
				i=8;
			}
			HAL_Delay(5);
		}

		if(btn_pressed==6 && prev_byte!=6) {
			return prev_byte;
		}
		else if(btn_pressed==6) {
			return btn_pressed;
		}
		else {
			prev_byte=btn_pressed;
		}


	}

  }

  void update_display(int secs)
  {
	// Write Hours with conditional leading zeros
	if (current_hours==0){
		write(decimal_numbers[0]); // 0
		write(decimal_numbers[0]); // 0
	} else if(current_hours<10) {
		write(decimal_numbers[0]); // 0
		write_binary_to_decimal(current_hours);
	} else { write_binary_to_decimal(current_hours); }

	write(0b01011100); // Write : separator

	// Write Minutes
	if (current_minutes==0) {
		write(decimal_numbers[0]); // 0
		write(decimal_numbers[0]); // 0

	} else if(current_minutes<10){
		write(decimal_numbers[0]); // 0

		write_binary_to_decimal(current_minutes);
	} else { write_binary_to_decimal(current_minutes); }

	// Write Seconds if secs==1; This Secs variable is used to "ignore" seconds in Time set and Alarm set menus.
	if(secs==1){
		write(0b01011100); // :
		if (current_seconds==0) {
			write(decimal_numbers[0]); // 0
			write(decimal_numbers[0]); // 0
		} else if(current_seconds<10){
			write(decimal_numbers[0]); // 0
			write_binary_to_decimal(current_seconds);
		} else { write_binary_to_decimal(current_seconds); }
	}
  }

  void update_time(){
	HAL_RTC_GetTime(&hrtc, &currentTime, RTC_FORMAT_BIN);//Get time
	HAL_RTC_GetDate(&hrtc, &currentDate, RTC_FORMAT_BIN);//get date
	current_hours=currentTime.Hours;
	current_minutes=currentTime.Minutes;
	current_seconds=currentTime.Seconds;
  }


  /**********************************************************/
  // User Interface functions:
  //	- Get_User_Input_Time()
  //	- Set_Time() submenu
  //	- Set_Alarm() submenu
  //	- Main_Menu() menu

  int get_user_input_time()
  {
	scroll_x_index=0;

	// Buffer each digit of hour & minutes
	if (current_hours==0){
		hour_digit1=0;
		hour_digit2=0;
	} else if(current_hours<10) {
		hour_digit1=0;
		hour_digit2=current_hours;
	} else {
		hour_digit1=current_hours/10;
		hour_digit2=current_hours%10;
	}
	if (current_minutes==0){
		minute_digit1=0;
		minute_digit2=0;
	} else if(current_minutes<10) {
		minute_digit1=0;
		minute_digit2=current_minutes;
	} else {
		minute_digit1=current_minutes/10;
		minute_digit2=current_minutes%10;
	}


	while(btn_pressed!=13) { // While BACK button not pressed; arbitrary integer "13" chosen as a de-bounce for back button when returning from sub-menus

		// Blink active digit
		if(current_seconds % 2==0){
			set_position(0x40);
			write_binary_to_decimal(hour_digit1);
			write_binary_to_decimal(hour_digit2);
			write(0b01011100); // :
			write_binary_to_decimal(minute_digit1);
			write_binary_to_decimal(minute_digit2);
		}
		else{
			set_position(0x40 + scroll_x_index); // Go to position of current digit
			write(0b10000001);
		}

		update_time();

		switch(process_inputs()) {
			case 1: { // Back pressed
				btn_pressed=13;
				break;
			}
			case 0: { // OK pressed

				return 1;
				break;
			}
			case 4: { // Right pressed; add to scroll X index with limits
				if(scroll_x_index<4){
					if(scroll_x_index+1==2){scroll_x_index+=1;}
					scroll_x_index+=1;
				}
				break;
			}
			case 3: { // Left pressed; subtract from scroll X index with limits
				if(scroll_x_index>0){
					if(scroll_x_index-1==2){scroll_x_index-=1;}
					scroll_x_index-=1;
				}
				break;
			}
			case 2: { // DOWN pressed
				switch(scroll_x_index) {
					case 0: {
						// Decrement if > 0
						if(hour_digit1>0){ hour_digit1-=1; }
						break;
					}
					case 1: {
						// Decrement if > 0
						if(hour_digit2>0){ hour_digit2-=1; }
						break;
					}
					// Skip index 2 - ":" character
					case 3: {
						// Decrement if > 0
						if(minute_digit1>0){ minute_digit1-=1; }
						break;
					}
					case 4: {
						// Decrement if > 0
						if(minute_digit2>0){minute_digit2-=1; }
						break;
					}
				}
				break;
			}
			case 5: { // UP pressed
				switch(scroll_x_index) {
					case 0: {
						// MSB Hour increment IFF <2
						if(hour_digit1<2){ hour_digit1+=1; }
						if(hour_digit1==2 && hour_digit2>3){ hour_digit2=3; }
						break;
					}
					case 1: {
						// LSB Hour increment up to 9 if MSB < 2, up to 4 if MSB==2
						if(hour_digit1<2 && hour_digit2<9) { hour_digit2+=1; }
						else if(hour_digit1==2 && hour_digit2<3) { hour_digit2+=1; }
						break;
					}
					case 3: {
						// MSB Minute increment IFF <5
						if(minute_digit1<5){ minute_digit1+=1;}
						break;
					}
					case 4: {
						// MSB Minute increment IFF <9
						if(minute_digit2<9){ minute_digit2+=1;}
						break;
					}
				}
				break;
			}
		}
	}
	return 0;
  }

  void set_time()
  {
	clear_home();
	write(0b11001010); // S
	write(0b10100010); // E
	write(0b00101010); // T
	write(0b10000001); // space
	write(0b00101010); // T
	write(0b10010010); // I
	write(0b10110010); // M
	write(0b10100010); // E
	set_position(0x40);
	update_display(0);
	if(get_user_input_time()==1)
	{
		currentTime.Hours = hacky_hex[(10*hour_digit1)+hour_digit2];
		currentTime.Minutes = hacky_hex[(10*minute_digit1)+minute_digit2];
		currentTime.Seconds = 0x00;
		currentTime.TimeFormat= RTC_HOURFORMAT12_PM;
		currentTime.DayLightSaving=RTC_DAYLIGHTSAVING_NONE;
		currentTime.StoreOperation=RTC_STOREOPERATION_RESET;
		HAL_RTC_SetTime(&hrtc, &currentTime, RTC_FORMAT_BCD);
		clear_home();
		write(0b00101010); // T
		write(0b10010010); // I
		write(0b10110010); // M
		write(0b10100010); // E
		write(0b10000001); // space
		write(0b11001010); // S
		write(0b10100010); // E
		write(0b00101010); // T
		write(0b10000100); // !
		HAL_Delay(1000);
	}
	clear_home();
  }

  void set_alarm()
  {
	clear_home();
	write(0b11001010); // S
	write(0b10100010); // E
	write(0b00101010); // T
	write(0b10000001); // space
	write(0b10000010); // A
	write(0b00110010); // L
	write(0b10000010); // A
	write(0b01001010); // R
	write(0b10110010); // M
	set_position(0x40);
	if(get_user_input_time()==1)
	{
		alarm_hour = (10*hour_digit1)+hour_digit2;
		alarm_minutes = (10*minute_digit1)+minute_digit2;
		clear_home();
		write(0b10000010); // A
		write(0b00110010); // L
		write(0b10000010); // A
		write(0b01001010); // R
		write(0b10110010); // M
		write(0b10000001); // space
		write(0b11001010); // S
		write(0b10100010); // E
		write(0b00101010); // T
		write(0b10000100); // !
		HAL_Delay(1000);
	}
	clear_home();
  }

  void main_menu(){
	while(btn_pressed!=10) {
		if(menu_text!=1){
			clear_home();
			write(0b10110010); // M
			write(0b10000010); // A
			write(0b10010010); // I
			write(0b01110010); // N
			write(0b10000001); // space
			write(0b10110010); // M
			write(0b10100010); // E
			write(0b10110010); // N
			write(0b10101010); // U
			set_position(0x40);

			switch(scroll_y_index) {
				case 0: {
					write_binary_to_decimal(scroll_y_index+1);
					write(0b01110100); // .
					write(0b10000001); // space
					write(0b11001010); // S
					write(0b10100010); // E
					write(0b00101010); // T
					write(0b10000001); // space
					write(0b00101010); // T
					write(0b10010010); // I
					write(0b10110010); // M
					write(0b10100010); // E
					break;
				}
				case 1: {
					write_binary_to_decimal(scroll_y_index+1);
					write(0b01110100); // .
					write(0b10000001); // space
					write(0b11001010); // S
					write(0b10100010); // E
					write(0b00101010); // T
					write(0b10000001); // space
					write(0b10000010); // A
					write(0b00110010); // L
					write(0b10000010); // A
					write(0b01001010); // R
					write(0b10110010); // M
					break;
				}
			}
			set_position(0x4F);
			write(0b01111110);
			menu_text=1;
		}
		switch(process_inputs()) {
			case 1: {
				menu_text=0;
				btn_pressed=10;
				break;
			}
			case 0: {
				menu_text=0;
				switch(scroll_y_index) {
					case 0: {
						scroll_y_index=0;
						set_time();
						break;
					}
					case 1: {
						scroll_y_index=0;
						set_alarm();
						break;
					}
				}

				break;
			}
			case 3: {
				menu_text=0;
				break;
			}
			case 4: {
				menu_text=0;
				break;
			}
			case 2: {
				menu_text=0;
				if(scroll_y_index<1)
				{
					scroll_y_index=scroll_y_index+1;
				}
				break;
			}
			case 5: {
				menu_text=0;
				if(scroll_y_index>0)
				{
					scroll_y_index=scroll_y_index-1;
				}
				break;
			}
		}
	}
	clear_home();
  }

  init_display();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	// Get user inputs
	switch(process_inputs()) {
		case 0: {
			// "OK"/menu button pressed
			main_menu();
			break;
		}
		case 1: {
			// "Back" button pressed - only used to disable during alarm
			// Disable alarm with Back button (ideally this will be a separate snooze/stop button)
			if(alarming==1)
			{
				alarming=0;
				snoozed=0;
				set_position(0x40);
				write(0b11001010); // S
				write(0b00101010); // T
				write(0b11110010); // O
				write(0b00001010); // P
				write(0b00001010); // P
				write(0b10100010); // E
				write(0b00100010); // D
				write(0b10000001); // space
				HAL_Delay(1000);
			}
			break;
		}
		case 5: {
			// "Up" button pressed - only used to snooze during alarm
			// Snooze
			if(alarming==1)
			{
				alarming=0;
				snoozed+=1;
				set_position(0x40);
				write(0b11001010); // S
				write(0b01110010); // N
				write(0b11110010); // O
				write(0b11110010); // O
				write(0b01011010); // Z
				write(0b10100010); // E
				write(0b00100010); // D
				write(0b10000001); // space
				write(0b10000001); // space
				write(0b10000001); // space
				write(0b10000001); // space
				HAL_Delay(1000);
			}
		}
	}

	update_time();

	// Set Alarming active upon alarm conditions
	// - Compares current time to user-set alarm time
	// - Adds 5-minutes to alarm-time for every time snooze is pressed
	// - Allows a 3-second buffer to ensure Main loop isn't stuck between second intervals (Total loop time can exceed 1 second)
	if( (current_hours==alarm_hour) & (current_minutes==alarm_minutes+(5*snoozed)) & (current_seconds<3))
	{
		alarming=1;
	}

	// If alarming, bit-bang square wave to Buzzer; note: no button interrupts, so relying on slight button-hold to capture button-press outside of this loop
	// To-do: Replace loop with Threading or use button interrupts (challenging with shift register)
	if(alarming==1) {
		for(int k=0; k<200; k++){
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 0);
			HAL_Delay(0.9);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 1);
			HAL_Delay(0.9);
		}
	}

	// Update display once per second
	if (current_seconds != last_seconds) {
		clear_home();
		update_display(1);
	}
	last_seconds = current_seconds;


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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA0 PA1 PA2 PA3
                           PA4 PA5 PA6 PA7
                           PA8 PA9 PA10 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB3 PB4 PB5 PB6 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
