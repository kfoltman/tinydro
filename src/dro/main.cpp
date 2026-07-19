#include "gpio.h"
#include "touchscreen.h"
#include "ili9488.h"
#include "gfx.h"
#include "app.h"
#include "buzzer.h"
#include "usbacm.h"
#include "ws2812b.h"
#include "i2c_keypad.h"

#define PA12 Pin{GPIOA, 12}

ILI9488LCD lcd;
App app;
USB_ACM serial;

#define LED1 Pin{GPIOC, 4}
#define LED2 Pin{GPIOC, 5}

extern "C" void SysTick_Handler(void)
{
    HAL_IncTick();
    app.updateEncoders();
    app.touchscreen.onSystick();
}

extern "C" void RCC_Handler(void)
{
}

extern "C" void HardFault_Handler(void)
{
    while(1) {
        LED2.setAsOutput().set();
        LED1.setAsOutput().set();
        HAL_Delay(100);
        LED2.setAsOutput().reset();
        LED1.setAsOutput().reset();
        HAL_Delay(100);
    }
}

extern "C" void WWDG_Handler(void)
{
#ifdef MCU_IS_STM32F4
    __HAL_RCC_PWR_CLK_ENABLE();
#endif
    __HAL_RCC_GPIOC_CLK_ENABLE();
    LED2.setAsOutput().set();
    LED1.setAsOutput().set();
}

extern "C" void Error_Handler(void)
{
    while(1) {}
}

#ifdef MCU_IS_STM32F4
void InitClocks()
{
    __HAL_RCC_PWR_CLK_ENABLE();    
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);    
    
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8; // for 2 MHz PLL input
    RCC_OscInitStruct.PLL.PLLN = 168;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};    
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV4; // 42 MHz
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;  // 21 MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        Error_Handler();
    }
    
    SYSCFG->CMPCR = 1;

    // Encoder clocks
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();
    // Buzzer timer
    HAL_BUZZER_CLK_ENABLE();
    // Display PWM
    __HAL_RCC_TIM12_CLK_ENABLE();
    // Touchscreen SPI
    __HAL_RCC_SPI2_CLK_ENABLE();
    // GPIOs
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    // Display bus
    __HAL_RCC_FMC_CLK_ENABLE();
    // USB
    __HAL_RCC_USB_OTG_FS_CLK_ENABLE();
}
#endif

#ifdef MCU_IS_STM32H7
void InitClocks()
{
    // All of this is based on UM2217

    // This is the built in LDO
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON; // Must stay on because the HSE/PLL is not set up yet
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;

    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 5; // 25 -> 5 MHz
    RCC_OscInitStruct.PLL.PLLN = 192; // 5 * 192 = 960
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    RCC_OscInitStruct.PLL.PLLP = 2; // 960 / 2 = 480
    RCC_OscInitStruct.PLL.PLLQ = 20; // 960 / 20 = 48 for usb
    RCC_OscInitStruct.PLL.PLLR = 2; // 960 / 2 = 480 unused
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2; // 4-8 MHz for PLLM output
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                                |RCC_CLOCKTYPE_D1PCLK1|RCC_CLOCKTYPE_D3PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1; // 480 MHz
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2; // 240 MHz
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;  // 120 MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_PeriphCLKInitTypeDef RCC_PeriphInitStruct = {0};
    RCC_PeriphInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USB;
    RCC_PeriphInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
    //RCC_PeriphInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
    if (HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    // MPU region for the TFT
    HAL_MPU_Disable();
    MPU_Region_InitTypeDef MPU_InitStruct;
    /* TEX0, C0, B0 = Strongly ordered */
    MPU_InitStruct.Number           = MPU_REGION_NUMBER0;
    MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress      = 0x60000000;
    MPU_InitStruct.Size             = MPU_REGION_SIZE_256MB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsCacheable      = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsShareable      = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
    // This may be needed for SCB_CleanDCache family functions to operate?
    SCB_InvalidateDCache();

    __HAL_RCC_CSI_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    HAL_EnableCompensationCell();

    // Encoder timers (quadrature decoders/counters)
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();
    // WS2812B timer
    __HAL_RCC_TIM5_CLK_ENABLE();
    // Buzzer timer
    HAL_BUZZER_CLK_ENABLE();
    // Display PWM
    __HAL_RCC_TIM12_CLK_ENABLE();
    // Touchscreen SPI
    __HAL_RCC_SPI2_CLK_ENABLE();
    // Touchscreen and WS2812B DMA
    __HAL_RCC_MDMA_CLK_ENABLE(); // not sure if needed?
    __HAL_RCC_DMA1_CLK_ENABLE();
    // Keypad
    __HAL_RCC_I2C1_CLK_ENABLE();
    // GPIOs
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    // Display bus
    __HAL_RCC_FMC_CLK_ENABLE();
    // USB
    HAL_PWREx_EnableUSBVoltageDetector();
    __HAL_RCC_USB2_OTG_FS_CLK_ENABLE();
}
#endif

void InitLEDs()
{
    // LEDs
    LED1.setAsOutput();
    LED2.setAsOutput();
    LED1.set();
    LED2.set();
}

int iters = 0;

void setup()
{
    HAL_Init();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    InitLEDs();
    InitClocks();
    
    LED1.reset();
    serial.init();
    lcd.init();
    app.init();
}

int resetTimer = 200;

void loop()
{
    globalTime++;
    if (globalTime & 128) {
        LED1.reset();
        LED2.set();
    } else {
        LED1.set();
        LED2.reset();
    }
    if (serial.isActive()) {
        if (serial.dtr()) {
            resetTimer = 0;
        } else {
            NVIC_SystemReset();
        }
    } else {
        // app.mainScreen.debug_label.setText("No serial");
        resetTimer = 200;
    }
    char buf[64];
    static int cnt = 0;
    sprintf(buf, "Var=%d cnt=%d\n", (int)app.eeprom.read32(0), cnt++);
    app.mainScreen.debug_label.setText(buf);
    Display display(lcd);
    app.render(display);
    app.loop();
#if 0
    if (!(globalTime & 15) && serial.isActive())
    {
        char buf[128];
        sprintf(buf, "%d %d %d\r\n", app.mainScreen.pos.coord(0), app.mainScreen.pos.coord(1), app.mainScreen.pos.coord(2));
        serial.write((const uint8_t *)buf, strlen(buf));
        serial.flush();
    }
    if (serial.isActive()) {
        while(serial.getRxCount() && serial.getTxSpace()) {
            uint8_t buf[256];
            uint32_t len = serial.read(buf, serial.getTxSpace());
            serial.write(buf, len);
        }
        if (!(globalTime & 15))
            serial.flush();
        serial.tryReceive();
    }
#endif
}

int main()
{
    setup();
    while(true)
        loop();
    return 0;
}

