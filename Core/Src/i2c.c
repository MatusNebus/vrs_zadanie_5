/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c.c
  * @brief   This file provides code for the configuration
  *          of the I2C instances.
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
#include "i2c.h"

/* USER CODE BEGIN 0 */
// maximalny pocet priechodov cakanim, nie cas v milisekundach
#define I2C_TIMEOUT 1000000U


// caka na priznak periferie; 0 = uspech, 1 = chyba
// static spristupni pomocnu funkciu iba v tomto subore
static uint8_t i2c_wait(uint32_t flag)
{
    uint32_t timeout = I2C_TIMEOUT;

    // isr je stavovy register stm32; maska flag vyberie pozadovany bit
    while (!(I2C1->ISR & flag))
    {
        // skonci pri nepotvrdeni prenosu, chybe zbernice alebo strate arbitraze
        if (I2C1->ISR & (I2C_ISR_NACKF |
                         I2C_ISR_BERR |
                         I2C_ISR_ARLO))
            return 1;

        // obmedzenie cakania zabrani nekonecnemu cyklu pri chybe
        if (--timeout == 0)
            return 1;
    }

    return 0;
}


// vymaze zaznamenany stop a chyby, aby neovplyvnili dalsi prenos
static void i2c_finish(void)
{
    LL_I2C_ClearFlag_STOP(I2C1);
    LL_I2C_ClearFlag_NACK(I2C1);
    LL_I2C_ClearFlag_BERR(I2C1);
    LL_I2C_ClearFlag_ARLO(I2C1);
}


// pri chybe sa pokusi ukoncit obsadenu zbernicu cez stop a vymaze priznaky
static void i2c_abort(void)
{
    if (LL_I2C_IsActiveFlag_BUSY(I2C1))
        LL_I2C_GenerateStopCondition(I2C1);

    i2c_finish();
}
/* USER CODE END 0 */

/* I2C1 init function */
void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  LL_I2C_InitTypeDef I2C_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
  /**I2C1 GPIO Configuration
  PB6   ------> I2C1_SCL
  PB7   ------> I2C1_SDA
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_6|LL_GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */

  /** I2C Initialization
  */
  LL_I2C_EnableAutoEndMode(I2C1);
  LL_I2C_DisableOwnAddress2(I2C1);
  LL_I2C_DisableGeneralCall(I2C1);
  LL_I2C_EnableClockStretching(I2C1);
  I2C_InitStruct.PeripheralMode = LL_I2C_MODE_I2C;
  I2C_InitStruct.Timing = 0x00201D2B;
  I2C_InitStruct.AnalogFilter = LL_I2C_ANALOGFILTER_ENABLE;
  I2C_InitStruct.DigitalFilter = 0;
  I2C_InitStruct.OwnAddress1 = 0;
  I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
  I2C_InitStruct.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;
  LL_I2C_Init(I2C1, &I2C_InitStruct);
  LL_I2C_SetOwnAddress2(I2C1, 0, LL_I2C_OWNADDRESS2_NOMASK);
  /* USER CODE BEGIN I2C1_Init 2 */
  // povolenie i2c periferie po jej nastaveni
  LL_I2C_Enable(I2C1);
  /* USER CODE END I2C1_Init 2 */

}

/* USER CODE BEGIN 1 */
// precita jeden alebo viac bajtov; 0 = uspech, 1 = chyba
// address je adresa senzora, reg je register, data je ciel a length pocet bajtov
uint8_t i2c_read(uint8_t address, uint8_t reg,
                 uint8_t *data, uint8_t length)
{
    // kontrola cieloveho pola a nenuloveho poctu bajtov
    if (data == 0 || length == 0)
        return 1;

    i2c_finish();

    // jeden bajt adresy registra; softend ponecha prenos bez stop
    LL_I2C_HandleTransfer(I2C1, address,
        LL_I2C_ADDRSLAVE_7BIT, 1,
        LL_I2C_MODE_SOFTEND,
        LL_I2C_GENERATE_START_WRITE);

    // txis oznacuje, ze periferia moze prevziat dalsi bajt na odoslanie
    if (i2c_wait(I2C_ISR_TXIS))
        goto error;

    // senzoru posleme adresu registra, s ktorym chceme pracovat
    LL_I2C_TransmitData8(I2C1, reg);

    // cakanie na dokoncenie odoslania adresy registra
    if (i2c_wait(I2C_ISR_TC))
        goto error;

    // opakovany start prepne na citanie; autoend vytvori stop po poslednom bajte
    LL_I2C_HandleTransfer(I2C1, address,
        LL_I2C_ADDRSLAVE_7BIT, length,
        LL_I2C_MODE_AUTOEND,
        LL_I2C_GENERATE_RESTART_7BIT_READ);


    // vsetky datove bajty spracujeme v ramci jedneho prenosu
    for (uint8_t i = 0; i < length; i++)
    {
        // rxne oznacuje prijaty bajt pripraveny na vybratie
        if (i2c_wait(I2C_ISR_RXNE))
            goto error;

        // vybratie prijateho bajtu z periferie do cieloveho pola
        data[i] = LL_I2C_ReceiveData8(I2C1);
    }

    // cakanie na zaznamenanie konca prenosu cez stop
    if (i2c_wait(I2C_ISR_STOPF))
        goto error;

    i2c_finish();
    return 0;

// spolocne ukoncenie pri chybe z lubovolnej fazy prenosu
error:
    i2c_abort();
    return 1;
}



// zapise jeden alebo viac bajtov; zdrojove data nemeni, 0 = uspech, 1 = chyba
uint8_t i2c_write(uint8_t address, uint8_t reg,
                  const uint8_t *data, uint8_t length)
{
    // limit je 255 bajtov na prenos, jeden z nich zabera adresa registra
    if (data == 0 || length == 0 || length > 254)
        return 1;

    i2c_finish();

    // jeden bajt adresy registra navyse k odosielanym datam
    LL_I2C_HandleTransfer(I2C1, address,
        LL_I2C_ADDRSLAVE_7BIT, length + 1,
        LL_I2C_MODE_AUTOEND,
        LL_I2C_GENERATE_START_WRITE);

    // txis oznacuje, ze periferia moze prevziat dalsi bajt na odoslanie
    if (i2c_wait(I2C_ISR_TXIS))
        goto error;

    // senzoru posleme adresu registra, s ktorym chceme pracovat
    LL_I2C_TransmitData8(I2C1, reg);

    // vsetky datove bajty spracujeme v ramci jedneho prenosu
    for (uint8_t i = 0; i < length; i++)
    {
        // txis oznacuje, ze periferia moze prevziat dalsi bajt na odoslanie
        if (i2c_wait(I2C_ISR_TXIS))
            goto error;

        LL_I2C_TransmitData8(I2C1, data[i]);
    }

    // cakanie na zaznamenanie konca prenosu cez stop
    if (i2c_wait(I2C_ISR_STOPF))
        goto error;

    i2c_finish();
    return 0;

// spolocne ukoncenie pri chybe z lubovolnej fazy prenosu
error:
    i2c_abort();
    return 1;
}
/* USER CODE END 1 */

