/**
  ******************************************************************************
  * @file    Demonstrations/Src/main.c 
  * @author  MCD Application Team
  * @brief   This demo describes how display bmp images from SD card on LCD using
             the Adafruit 1.8" TFT shield with Joystick and microSD mounted on top
             of the STM32 Nucleo board.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
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

/** @addtogroup STM32F0xx_HAL_Demonstrations
  * @{
  */

/** @addtogroup Demo
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
typedef enum 
{
  SHIELD_NOT_DETECTED = 0, 
  SHIELD_DETECTED
}ShieldStatus;

/* Private define ------------------------------------------------------------*/
#define SD_CARD_NOT_FORMATTED                    0
#define SD_CARD_FILE_NOT_SUPPORTED               1
#define SD_CARD_OPEN_FAIL                        2
#define FATFS_NOT_MOUNTED                        3
#define BSP_SD_INIT_FAILED                       4

#define POSITION_X_BITMAP                        0
#define POSITION_Y_BITMAP                        0

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t BlinkSpeed = 0, str[20];
__IO uint8_t JoystickValue = 0;
char* pDirectoryFiles[MAX_BMP_FILES];
FATFS SD_FatFs;  /* File system object for SD card logical drive */
char SD_Path[4]; /* SD card logical drive path */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void LED2_Blink(void);
static ShieldStatus TFT_ShieldDetect(void);
static void SDCard_Config(void);
static void TFT_DisplayErrorMessage(uint8_t message);
static void TFT_DisplayMenu(void);
static void TFT_DisplayImages(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{  
  /* STM32F0xx HAL library initialization:
       - Configure the Flash prefetch
       - Systick timer is configured by default as source of time base, but user 
         can eventually implement his proper time base source (a general purpose 
         timer for example or other time source), keeping in mind that Time base 
         duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and 
         handled in milliseconds basis.
       - Low Level Initialization
     */
  HAL_Init();  
  
  /* Configure the system clock = 48 MHz */
  SystemClock_Config();
  
  /* Check the availability of adafruit 1.8" TFT shield on top of STM32NUCLEO
     board. This is done by reading the state of IO PB.00 pin (mapped to JoyStick
     available on adafruit 1.8" TFT shield). If the state of PB.00 is high then
     the adafruit 1.8" TFT shield is available. */  
  if(TFT_ShieldDetect() == SHIELD_DETECTED)
  {
    /* Initialize the LCD */
    BSP_LCD_Init();
    
    /* Configure SD card */
    SDCard_Config(); 
    
    /* Display on TFT Images existing on SD card */
    TFT_DisplayImages();
  }  
  else /* Shield not mounted */
  {   
    LED2_Blink();
  } 
  
  /* Infinite loop */
  while (1)
  {
  }
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSI/2)
  *            SYSCLK(Hz)                     = 48000000
  *            HCLK(Hz)                       = 48000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 1
  *            HSI Frequency(Hz)              = 8000000
  *            PREDIV                         = 1
  *            PLLMUL                         = 12
  *            Flash Latency(WS)              = 1
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  
  /* No HSE Oscillator on Nucleo, Activate PLL with HSI/2 as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_NONE;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct)!= HAL_OK)
  {
    /* Initialization Error */
    while(1); 
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1)!= HAL_OK)
  {
    /* Initialization Error */
    while(1); 
  }
}


/**
  * @brief  Displays demonstration menu.
  * @param  None
  * @retval None
  */
static void TFT_DisplayMenu(void)
{
  JOYState_TypeDef tmp = JOY_NONE;
  
  /* Set Menu font */
  BSP_LCD_SetFont(&Font12);

  /* Set Text color */
  BSP_LCD_SetTextColor(LCD_COLOR_RED);
  /* Display message */
  BSP_LCD_DisplayStringAtLine(1, (uint8_t*)" NUCLEO-STM32F0xx   ");
  BSP_LCD_DisplayStringAtLine(2, (uint8_t*)"       DEMO         ");
  
  /* Set Text color */
  BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
  /* Display message */  
  BSP_LCD_DisplayStringAtLine(4, (uint8_t*)" Display images      ");
  BSP_LCD_DisplayStringAtLine(6, (uint8_t*)" stored under uSD    ");
  BSP_LCD_DisplayStringAtLine(8, (uint8_t*)" on TFT LCD          ");
  
  /* Set Text color */
  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  /* Display message */ 
  BSP_LCD_DisplayStringAtLine(11, (uint8_t*)"  Press JOY DOWN   ");
  BSP_LCD_DisplayStringAtLine(12, (uint8_t*)"  to continue...   ");
 
  /* Wait for JOY_DOWN is pressed */
  while (BSP_JOY_GetState() != JOY_DOWN)
  {
  }
  
  /* Wait for JOY_DOWN is released */
  while (BSP_JOY_GetState() == JOY_DOWN)
  {
  }
  
  /* Set Text color */
  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);  
  /* Display message */ 
  BSP_LCD_DisplayStringAtLine(4,  (uint8_t*)"                   ");
  BSP_LCD_DisplayStringAtLine(6,  (uint8_t*)"  Press Joystick   ");
  
  /* Set Text color */
  BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
  /* Display message */ 
  BSP_LCD_DisplayStringAtLine(8,  (uint8_t*)"  UP for:          ");
  BSP_LCD_DisplayStringAtLine(9,  (uint8_t*)"  Manual Mode      ");
  BSP_LCD_DisplayStringAtLine(11, (uint8_t*)"  DOWN for:        ");
  BSP_LCD_DisplayStringAtLine(12, (uint8_t*)"  Automatic Mode   ");

  /* Wait for JOY_DOWN or JOY_UP is pressed */
  tmp = JOY_RIGHT;
  while ((tmp != JOY_DOWN) && (tmp != JOY_UP))
  {
    tmp = BSP_JOY_GetState();
  }
  
  /* LCD Clear */
  BSP_LCD_Clear(LCD_COLOR_WHITE); 
  
  /* JOY_UP is pressed: Display Manual mode menu #############################*/
  if(tmp == JOY_UP)
  {
    /* Set Text color */
    BSP_LCD_SetTextColor(LCD_COLOR_RED);    
    /* Display message */ 
    BSP_LCD_DisplayStringAtLine(3,  (uint8_t*)"   Manual Mode   ");
    BSP_LCD_DisplayStringAtLine(5,  (uint8_t*)"    Selected     "); 
    
    /* Set Text color */
    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);      
    /* Display message */
    BSP_LCD_DisplayStringAtLine(9,  (uint8_t*)" RIGHT: Next image"); 
    BSP_LCD_DisplayStringAtLine(10, (uint8_t*)" LEFT : Previous  ");
    BSP_LCD_DisplayStringAtLine(11, (uint8_t*)" SEL  : Switch to ");
    BSP_LCD_DisplayStringAtLine(12, (uint8_t*)" automatic mode   ");    
    JoystickValue = 2;  
  }
  /* JOY_DOWN is pressed: Display Automatic mode menu ########################*/
  else if (tmp == JOY_DOWN)    
  {
    /* Set Text color */
    BSP_LCD_SetTextColor(LCD_COLOR_RED);
    /* Display message */ 
    BSP_LCD_DisplayStringAtLine(3,  (uint8_t*)"  Automatic Mode  ");
    BSP_LCD_DisplayStringAtLine(5,  (uint8_t*)"     Selected     ");
    

    JoystickValue = 1;  
    HAL_Delay(200);
  }
}

/**
  * @brief  Displays on TFT Images or error messages when error occurred.
  * @param  None
  * @retval None
  */
static void TFT_DisplayImages(void)
{    
  uint32_t bmplen = 0x00;
  uint32_t checkstatus = 0x00;
  uint32_t filesnumbers = 0x00;
  uint32_t joystickstatus = JOY_NONE;
  uint32_t bmpcounter = 0x00;
  uint32_t tickstart;
  DIR directory;
  FRESULT res;
  
  /* Initialize the Joystick available on adafruit 1.8" TFT shield */
  BSP_JOY_Init();
  
  /* Welcome message */
  TFT_DisplayMenu(); 
  
  /* Open directory */
  res = f_opendir(&directory, "/");
  if((res != FR_OK))
  {
    if(res == FR_NO_FILESYSTEM)
    {
      /* Display message: SD card not FAT formated */
      TFT_DisplayErrorMessage(SD_CARD_NOT_FORMATTED);    
    }
    else
    {
      /* Display message: Fail to open directory */
      TFT_DisplayErrorMessage(SD_CARD_OPEN_FAIL);  
    }
  }
  
  /* Get number of bitmap files */
  filesnumbers = Storage_GetDirectoryBitmapFiles ("/", pDirectoryFiles);    
  /* Set bitmap counter to display first image */
  bmpcounter = 1;
  tickstart = HAL_GetTick();
  
  while (1)
  {     
    /* Ensure a small tempo between images display unless a user action occurs */
    while ( ((HAL_GetTick() - tickstart) < 1500) && (joystickstatus == JOY_NONE) )
    {
      /* Get JoyStick status */    
      joystickstatus = BSP_JOY_GetState();
      HAL_Delay(200);
    }
    
    if(joystickstatus == JOY_SEL)
    {      
      JoystickValue++;
      if (JoystickValue > 2)
      {
        JoystickValue = 1;
      }
    }
    
    tickstart = HAL_GetTick();

    /*## Display BMP pictures in Automatic mode ##############################*/
    if(JoystickValue == 1) 
    {
      sprintf((char*)str, "%-11.11s", pDirectoryFiles[bmpcounter -1]);
      
      checkstatus = Storage_CheckBitmapFile((const char*)str, &bmplen);
      
      if(checkstatus == 0)
      {
        /* Format the string */
        checkstatus = Storage_OpenReadFile(POSITION_X_BITMAP, POSITION_Y_BITMAP, (const char*)str);
      }
      
      if (checkstatus == 1)
      {
        /* Display message: File not supported */
        TFT_DisplayErrorMessage(SD_CARD_FILE_NOT_SUPPORTED);
      }

      bmpcounter++;
      if(bmpcounter > filesnumbers)
      {
        bmpcounter = 1;
      }
    }   
    
    /*## Display BMP pictures in Manual mode #################################*/
    if(JoystickValue == 2)
    {
      if(joystickstatus == JOY_RIGHT)
      {
        if((bmpcounter + 1) > filesnumbers)
        {
          bmpcounter = 1;
        }
        else
        {
          bmpcounter++;
        }
        sprintf ((char*)str, "%-11.11s", pDirectoryFiles[bmpcounter - 1]);
        
        checkstatus = Storage_CheckBitmapFile((const char*)str, &bmplen);
        
        if(checkstatus == 0)
        {
          /* Format the string */
          Storage_OpenReadFile(POSITION_X_BITMAP, POSITION_Y_BITMAP, (const char*)str); 
        }
        
        if(checkstatus == 1)
        {
          /* Display message: File not supported */
          TFT_DisplayErrorMessage(SD_CARD_FILE_NOT_SUPPORTED); 
        }
        JoystickValue = 2;          
      }
      else if(joystickstatus == JOY_LEFT)
      {
        if((bmpcounter - 1) == 0)
        {
          bmpcounter = filesnumbers;
        }
        else
        {
          bmpcounter--;
        }
        sprintf ((char*)str, "%-11.11s", pDirectoryFiles[bmpcounter - 1]); 
        checkstatus = Storage_CheckBitmapFile((const char*)str, &bmplen);
        
        if(checkstatus == 0)
        {
          /* Format the string */
          Storage_OpenReadFile(POSITION_X_BITMAP, POSITION_Y_BITMAP, (const char*)str); 
        }
        
        if (checkstatus == 1)
        {
          /* Display message: File not supported */
          TFT_DisplayErrorMessage(SD_CARD_FILE_NOT_SUPPORTED);
        }
        JoystickValue = 2;
      }
    }

    joystickstatus = JOY_NONE;
  }  
}

/**
  * @brief  SD Card Configuration.
  * @param  None
  * @retval None
  */
static void SDCard_Config(void)
{
  uint32_t counter = 0;
  
  if(FATFS_LinkDriver(&SD_Driver, SD_Path) == 0)
  {
    /* Initialize the SD mounted on adafruit 1.8" TFT shield */
    if(BSP_SD_Init() != MSD_OK)
    {
      TFT_DisplayErrorMessage(BSP_SD_INIT_FAILED);
    }  
    
    /* Check the mounted device */
    if(f_mount(&SD_FatFs, (TCHAR const*)"/", 0) != FR_OK)
    {
      TFT_DisplayErrorMessage(FATFS_NOT_MOUNTED);
    }  
    else
    {
      /* Initialize the Directory Files pointers (heap) */
      for (counter = 0; counter < MAX_BMP_FILES; counter++)
      {
        pDirectoryFiles[counter] = malloc(11); 
      }
    }
  }
}

/**
  * @brief  Displays adequate message on TFT available on adafruit 1.8" TFT shield  
  * @param  message: Error message to be displayed on the LCD.
  *   This parameter can be one of following values:   
  *     @arg SD_CARD_NOT_FORMATTED: SD CARD is not FAT formatted
  *     @arg SD_CARD_FILE_NOT_SUPPORTED: File is not supported
  *     @arg SD_CARD_OPEN_FAIL: Failure to open directory
  *     @arg FATFS_NOT_MOUNTED: FatFs is not mounted
  * @retval None
  */
static void TFT_DisplayErrorMessage(uint8_t message)
{
  /* LCD Clear */
  BSP_LCD_Clear(LCD_COLOR_WHITE); 
  /* Set Error Message Font */
  BSP_LCD_SetFont(&Font12);
  /* Set Text and Back colors */
  BSP_LCD_SetBackColor(LCD_COLOR_GREY); 
  BSP_LCD_SetTextColor(LCD_COLOR_RED);

  if(message == SD_CARD_NOT_FORMATTED)
  {
    /* Display message */
    BSP_LCD_DisplayStringAtLine(5, (uint8_t*)" SD Card is not    ");
    BSP_LCD_DisplayStringAtLine(6, (uint8_t*)" FAT formatted.    ");  
    BSP_LCD_DisplayStringAtLine(7, (uint8_t*)" Please Format the ");
    BSP_LCD_DisplayStringAtLine(8, (uint8_t*)" microSD card.     ");
    while (1)
    {
    }    
  }
  if(message == SD_CARD_FILE_NOT_SUPPORTED)
  {
    /* Display message */
    BSP_LCD_DisplayStringAtLine(5, (uint8_t*)"                   ");
    BSP_LCD_DisplayStringAtLine(6, (uint8_t*)" File type is not  ");
    BSP_LCD_DisplayStringAtLine(7, (uint8_t*)" supported.        ");
    BSP_LCD_DisplayStringAtLine(8, (uint8_t*)"                   ");
    while(1)
    {
    }    
  }
  if(message == SD_CARD_OPEN_FAIL)
  {
    /* Display message */
    BSP_LCD_DisplayStringAtLine(5, (uint8_t*)"                   ");
    BSP_LCD_DisplayStringAtLine(6, (uint8_t*)" Open directory    ");
    BSP_LCD_DisplayStringAtLine(7, (uint8_t*)" fails.            ");
    BSP_LCD_DisplayStringAtLine(8, (uint8_t*)"                   ");
    while(1)
    {
    }     
  }
  if(message == FATFS_NOT_MOUNTED)
  {
    /* Display message */
    BSP_LCD_DisplayStringAtLine(5, (uint8_t*)"                   ");
    BSP_LCD_DisplayStringAtLine(6, (uint8_t*)" Cannot mount      ");
    BSP_LCD_DisplayStringAtLine(7, (uint8_t*)" FatFs on Drive.   "); 
    BSP_LCD_DisplayStringAtLine(8, (uint8_t*)"                   ");
    while (1)
    {
    }  
  }
  if(message == BSP_SD_INIT_FAILED)
  {
    /* Display message */
    BSP_LCD_DisplayStringAtLine(5, (uint8_t*)"                   ");
    BSP_LCD_DisplayStringAtLine(6, (uint8_t*)" SD Init           ");
    BSP_LCD_DisplayStringAtLine(7, (uint8_t*)" fails.            ");
    BSP_LCD_DisplayStringAtLine(8, (uint8_t*)"                   ");
    while(1)
    {
    }     
  }
}

/**
  * @brief  Blinks LED2 with two frequencies depending on User press button.
  * @param  None
  * @retval None
  */
static void LED2_Blink(void)
{
  /* Configure LED2 on Nucleo */
  BSP_LED_Init(LED2);

  /* Configure the User Button in EXTI Mode */
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);
  
  /* Initiate BlinkSpeed variable */ 
  BlinkSpeed = 0;  
  
  /* Infinite loop */
  while(1)
  {
    /* Test on blink speed */
    if(BlinkSpeed == 0)
    {
      BSP_LED_Toggle(LED2);
      /* Wait for 500ms */      
      HAL_Delay(500);      
    }      
    else if(BlinkSpeed == 1)
    {
      BSP_LED_Toggle(LED2);
      /* Wait for 100ms */
      HAL_Delay(100); 
    }
    else if(BlinkSpeed == 2)
    {
      BSP_LED_Toggle(LED2);    
      /* wait for 50ms */
      HAL_Delay(50);  
    }
  }
}

/**
  * @brief  EXTI line detection callbacks.
  * @param  GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if(BlinkSpeed == 2)
  {
    BlinkSpeed = 0;
  }
  else
  {
    BlinkSpeed ++;
  }
}

/**
  * @brief  Check the availability of adafruit 1.8" TFT shield on top of STM32NUCLEO
  *         board. This is done by reading the state of IO PB.00 pin (mapped to 
  *         JoyStick available on adafruit 1.8" TFT shield). If the state of PB.00 
  *         is high then the adafruit 1.8" TFT shield is available.
  * @param  None
  * @retval SHIELD_DETECTED: 1.8" TFT shield is available
  *         SHIELD_NOT_DETECTED: 1.8" TFT shield is not available
  */
static ShieldStatus TFT_ShieldDetect(void)
{
  GPIO_InitTypeDef  GPIO_InitStruct; 

  /* Enable GPIO clock */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) != 0)
  {
    return SHIELD_DETECTED;
  }
  else
  {
    return SHIELD_NOT_DETECTED;
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(char* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/