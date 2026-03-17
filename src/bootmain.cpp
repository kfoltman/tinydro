#include "buzzer.h"
#include "gpio.h"
#include "usbacm.h"

Buzzer buzzer;
USB_ACM serial;
volatile int timeSinceLastChar = 0;

extern "C" void SysTick_Handler(void)
{
    HAL_IncTick();
    timeSinceLastChar += 1;
}

extern "C" void RCC_Handler(void)
{
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
        while(1) {}
    }
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};    
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV4;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        while(1) {}
    }
    
    SYSCFG->CMPCR = 1;

    // Buzzer timer
    __HAL_RCC_TIM9_CLK_ENABLE();
    // Display PWM
    __HAL_RCC_TIM12_CLK_ENABLE();
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

void startApp()
{
    serial.deinit();
    HAL_Delay(100);
    __HAL_RCC_TIM9_CLK_DISABLE();
    // Display PWM
    __HAL_RCC_TIM12_CLK_DISABLE();
#ifdef STM32F427xx
    __HAL_RCC_FMC_CLK_DISABLE();
#else
    __HAL_RCC_FSMC_CLK_DISABLE();
#endif
    __HAL_RCC_USB_OTG_FS_CLK_DISABLE();
    __HAL_RCC_USB_OTG_FS_FORCE_RESET();
    buzzer.beep(880, 5);
    __HAL_RCC_USB_OTG_FS_RELEASE_RESET();
    uint32_t *app_ivec = (uint32_t *)0x08020000;
    uint32_t *sram_ivec = (uint32_t *)0x20000000;
    for (int i = 0; i < 256; ++i)
        sram_ivec[i] = app_ivec[i];
        
    SYSCFG->MEMRMP = 3; // remap interrupt vectors
    typedef void (*ResetFunc)(void);
    ResetFunc resetFunc = (ResetFunc)sram_ivec[1];
    resetFunc();
}

void eraseSectors(int sector, int nsectors)
{
    FLASH_EraseInitTypeDef erase_data;
    erase_data.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_data.Banks = FLASH_BANK_1;
    erase_data.Sector = FLASH_SECTOR_0 + sector; // bootloader covers sectors 0-1, sectors 2-3 are for EEPROM sim, sectors 4-end are for the application
    erase_data.NbSectors = nsectors;
    erase_data.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    
    uint32_t sector_error = 0;
    HAL_FLASH_Unlock();
    int status = HAL_FLASHEx_Erase(&erase_data, &sector_error);
    HAL_FLASH_Lock();
    if (status == HAL_OK) {
        serial.writestr("OK\r\n");
    } else {
        serial.writestr("Failed\r\n");
        serial.writehex(sector_error);
        serial.writestr("\r\n");
    }
    serial.flush();
}

void getAppChecksum()
{
    uint32_t checksum = 0;
    const uint32_t *app_start = (uint32_t *)0x08020000;
    const uint32_t *app_end = (uint32_t *)0x08060000;
    for (const uint32_t *p = app_start; p != app_end; ++p) {
        checksum += *p;
    }
    serial.writestr("Checksum=");
    serial.writehex(checksum);
    serial.writestr("\r\n");
    serial.flush();
}

static inline bool isxdigit(uint8_t ch)
{
    return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F');
}

static inline uint8_t fromhex(uint8_t ch)
{
    return (ch >= 'A') ? (ch - 'A' + 10) : (ch - '0');
}

void programFlash()
{
    char line[96];
    uint32_t pos = 0;
    do {
        if (pos >= sizeof(line)) {
            serial.writestr("Overflow\r\n");
            serial.flush();
            return;
        }
        while(!serial.read((uint8_t *)&line[pos], 1)) {
        }
        if (line[pos] == '\r')
            break;
        if (line[pos] >= 'a' && line[pos] <= 'z')
            line[pos] -= 'a' - 'A';
        pos++;
    } while(1);
    line[pos] = '\0';
    uint32_t addr = 0;
    pos = 0;
    while(isxdigit(line[pos])) {
        addr = addr * 16U + fromhex(line[pos]);
        pos++;
    }
    if (line[pos] != ':') {
        serial.writestr("BadChar\r\n");
        serial.flush();
        return;
    }
    pos++;
    HAL_FLASH_Unlock();
    union {
        uint32_t dataWord;
        uint8_t dataByte[4];
    } data;
    data.dataWord = 0;
    uint8_t cycle = 0;
    uint32_t base = 0x08020000;
    while(isxdigit(line[pos])) {
        data.dataByte[cycle >> 1] = data.dataByte[cycle >> 1] * 16U + fromhex(line[pos]);
        cycle++;
        if (cycle == 8) {
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, base + addr, data.dataWord);
            addr += 4;
            data.dataWord = 0;
            cycle = 0;
        }
        pos++;
    }
    HAL_FLASH_Lock();
    if (line[pos] != '\0') {
        serial.writestr("BadEnd\r\n");
        serial.flush();
        return;
    }
    serial.writestr("OK\r\n");
    serial.flush();
}

int main()
{
    InitClocks();
    HAL_Init();
    
    buzzer.beep(440, 5);
    serial.init();
    
    while (timeSinceLastChar < 2000) {
        if (serial.is_active()) {
            uint8_t ch;
            if (serial.read(&ch, 1)) {
                serial.write(&ch, 1);
                serial.flush();
                if (ch == 'b')
                    startApp();
                if (ch == 'c')
                    getAppChecksum();
                if (ch == 'e')
                    eraseSectors(2, 2);
                if (ch == 'E')
                    eraseSectors(4, 4);
                if (ch == 'Q') {
                    serial.writestr("Echo\r\n");
                    serial.flush();
                }
                if (ch == 'p') {
                    programFlash();
                }
                timeSinceLastChar = 0;
            }
        }
        HAL_Delay(1);
    }
    startApp();
    return 0;
}
