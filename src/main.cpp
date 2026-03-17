#include "gpio.h"
#include "touchscreen.h"
#include "ili9488.h"
#include "gfx.h"
#include "app.h"
#include "buzzer.h"
#include "usbacm.h"

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

extern "C" void WWDG_Handler(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    LED2.setAsOutput().set();
    LED1.setAsOutput().set();
}

extern "C" void Error_Handler(void)
{
    while(1) {}
}

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
    __HAL_RCC_TIM9_CLK_ENABLE();
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
#ifdef STM32F427xx
    __HAL_RCC_FMC_CLK_ENABLE();
#else
    __HAL_RCC_FSMC_CLK_ENABLE();
#endif
    // USB
    __HAL_RCC_USB_OTG_FS_CLK_ENABLE();
}

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
    InitClocks();
    HAL_Init();
    InitLEDs();
    
    serial.init();
    lcd.init();
    app.init();
}

int resetTimer = 200;

void loop()
{
    globalTime++;
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
    if (!(globalTime & 15) && serial.isActive())
    {
        char buf[128];
        sprintf(buf, "%d %d %d\r\n", app.mainScreen.pos.coord(0), app.mainScreen.pos.coord(1), app.mainScreen.pos.coord(2));
        serial.write((const uint8_t *)buf, strlen(buf));
        serial.flush();
    }
#if 0
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

