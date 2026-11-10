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
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
// rozhrania vlastnych senzorov a snprintf na prevod cisel na text
#include "lps25hb.h"
#include "hts221.h"
#include <stdio.h>
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

/* USER CODE BEGIN PV */
// stav lps25hb: 0 = uspech, 1 = chyba alebo zatial neinicializovany senzor
uint8_t sensor_ok = 1;
// aktualny tlak, pociatocny referencny tlak a vyska oproti pociatocnej polohe
float pressure_hpa = 0;
float start_pressure = 0;
float height_m = 0;

// stav hts221: 0 = uspech, 1 = chyba alebo zatial neinicializovany senzor
uint8_t hts_ok = 1;
// teplota v stupnoch celzia a relativna vlhkost v percentach
float temperature_c = 0;
float humidity_percent = 0;

// miesto pre csv riadok vratane ukoncovacieho znaku textu
char message[80];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// posle text cez usart2; do pc ide cez seriovy prevodnik dosky
void usart_send(char *text)
{
    // nulovy znak oznacuje koniec textu a uz sa neposiela
    while (*text)
    {
        // cakanie na volne miesto vo vysielacom registri
        while (!LL_USART_IsActiveFlag_TXE(USART2));

        // odoslanie aktualneho znaku a posun ukazovatela na dalsi znak
        LL_USART_TransmitData8(USART2, *text++);
    }

    // cakanie na uplne odvysielanie posledneho znaku
    while (!LL_USART_IsActiveFlag_TC(USART2));
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
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  /* System interrupt init*/
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /* SysTick_IRQn interrupt configuration */
  NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),15, 0));

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  // inicializacia oboch senzorov a odovzdanie i2c funkcii ako callbackov
  sensor_ok = lps25hb_init(i2c_read, i2c_write);
  hts_ok = hts221_init(i2c_read, i2c_write);
  
  if (sensor_ok == 0)
  {
      // pri merani 1 hz nechame senzoru cas vytvorit prvy vysledok
      LL_mDelay(1200);
      // & odovzda adresu premennej, do ktorej sa ulozi tlak pre vyskovu nulu
      sensor_ok = lps25hb_read_pressure(&start_pressure);
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    if (sensor_ok == 0)
    {
      // tlak sa ulozi cez ukazovatel, navratova hodnota oznamuje stav citania
      // po chybe sa dalsie citanie uz neskusa, automaticka obnova tu nie je
      sensor_ok = lps25hb_read_pressure(&pressure_hpa);

      if (sensor_ok == 0)
      {
        // vypocet vysky oproti pociatocnemu tlaku iba po uspesnom citani
        height_m = lps25hb_relative_height(
            pressure_hpa, start_pressure);
      }
    }

    // samostatne nacitanie teploty a vlhkosti, ak senzor doteraz nezlyhal
    if (hts_ok == 0)
    {
        // pri chybe teploty sa vdaka || uz citanie vlhkosti nevykona
        if (hts221_read_temperature(&temperature_c) != 0 ||
            hts221_read_humidity(&humidity_percent) != 0)
        {
            // chyba zastavi dalsie citanie tohto senzora
            hts_ok = 1;
        }
    }

    // pockame priblizne sekundu; hodnoty sa odoslu az po tomto cakani
    LL_mDelay(1000);

    // riadok posleme iba vtedy, ked su oba senzory bez chyby
    if (sensor_ok == 0 && hts_ok == 0)
    {
      // csv: teplota na 1 miesto, vlhkost bez desatin, tlak a vyska na 2 miesta
      // sizeof obmedzi velkost textu; \r\n ukonci riadok, snprintf ho este neodosiela
      snprintf(message, sizeof(message),
               "%.1f,%.0f,%.2f,%.2f\r\n",
               temperature_c,
               humidity_percent,
               pressure_hpa,
               height_m);

      // hotovy textovy riadok odosleme cez seriovu linku do pc
      usart_send(message);
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
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_0)
  {
  }
  LL_RCC_HSI_Enable();

   /* Wait till HSI is ready */
  while(LL_RCC_HSI_IsReady() != 1)
  {

  }
  LL_RCC_HSI_SetCalibTrimming(16);
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI)
  {

  }
  LL_Init1msTick(8000000);
  LL_SetSystemCoreClock(8000000);
  LL_RCC_SetI2CClockSource(LL_RCC_I2C1_CLKSOURCE_HSI);
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
