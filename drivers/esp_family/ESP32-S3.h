#pragma once
#include "ESP32_DMA.h"
#include <freertos/FreeRTOS.h> //Maybe move if needed
#include <freertos/task.h>
#include <esp_log.h>

static const char *DISPLAY_TAG = "DISPLAY";

// --- GPIO registers ---  ESP32
#define GPIO_OUT_W1TS_REG (*((volatile uint32_t *)0x60004008))
#define GPIO_OUT_W1TC_REG (*((volatile uint32_t *)0x6000400C))
#define GPIO_ENABLE_REG   (*((volatile uint32_t *)0x60004020))
#define GPIO_ENABLE_W1TC_REG (*((volatile uint32_t *)0x60004028))
#define GPIO_IN_REG       (*((volatile uint32_t *)0x6000403C))

// --- GPIO matrix output routing (base 0x60004000 + 0x0554 + 4*n) ---
#define GPIO_FUNC_OUT_SEL_BASE  0x60004554
#define GPIO_OUT_SEL(n)  (*((volatile uint32_t *)(GPIO_FUNC_OUT_SEL_BASE + (n)*4)))

#define GPIO_ENABLE_W1TS_REG (*((volatile uint32_t *)0x60004024))



//Keep in mind, Since the two pins are pulled up through IO_MUX,
//It should go to zero (ACTIVE LOW)
// #define INPUT_LEFT (((GPIO_IN_REG>>GPIO_BTN_1) & 1) == 0) 
// #define INPUT_RIGHT (((GPIO_IN_REG>> GPIO_BTN_2) & 1) == 0)
#define INPUT_RUN (((GPIO_IN_REG>> GPIO_BTN_2) & 1) == 0)
#define INPUT_INTERACT (((GPIO_IN_REG>> GPIO_BTN_2) & 1) == 0)
#define INPUT_JUMP (((GPIO_IN_REG>>GPIO_BTN_4) & 1) == 0)

// --- IO_MUX registers --- ESP32-S3
//0x60009000 base address for IO_MUX
#define IO_MUX_GPIO10_REG   (*((volatile uint32_t*)0x6000902C)) //Chip Select
#define IO_MUX_GPIO11_REG  (*((volatile uint32_t*)0x60009030))  //DIN (MOSI)
#define IO_MUX_GPIO12_REG  (*((volatile uint32_t*)0x60009034))  //SPI_CLK
#define IO_MUX_GPIO13_REG  (*((volatile uint32_t*)0x60009038))  //Data/Command (GPIO)
#define IO_MUX_GPIO9_REG  (*((volatile uint32_t*)0x60009028))  //RESET PIN
#define IO_MUX_GPIO14_REG  (*((volatile uint32_t*)0x6000903C))  //BL PIN

#define IO_MUX_GPIO21_REG  (*((volatile uint32_t*)0x60009058))  //General Button Input 1
#define IO_MUX_GPIO1_REG   (*((volatile uint32_t*)0x60009008))  //General Button Input 2
#define IO_MUX_GPIO2_REG   (*((volatile uint32_t*)0x6000900C))  //General Button Input 3
#define IO_MUX_GPIO4_REG   (*((volatile uint32_t*)0x60009014))  //General Button Input 4


//I believe this was for I2S Audio interfacing
#define IO_MUX_GPIO16_REG   (*((volatile uint32_t*)0x60009044))
#define IO_MUX_GPIO17_REG   (*((volatile uint32_t*)0x60009048))
#define IO_MUX_GPIO18_REG   (*((volatile uint32_t*)0x6000904C))



// --- SPI2 (VSPI) registers ---
//0x60024000 SPI2 Controller
#define SPI2_CMD_REG        (*((volatile uint32_t *)0x60024000))
#define SPI2_USER_REG       (*((volatile uint32_t *)0x60024010))
#define SPI2_USER1_REG      (*((volatile uint32_t *)0x60024014))
#define SPI2_CTRL_REG       (*((volatile uint32_t *)0x60024008))
#define SPI2_CLK_GATE_REG   (*((volatile uint32_t *)0x600240E8))
#define SPI2_CLOCK_REG      (*((volatile uint32_t *)0x6002400C))
#define SPI2_MS_DLEN_REG    (*((volatile uint32_t *)0x6002401C))
#define SPI2_MISC_REG       (*((volatile uint32_t *)0x60024020)) //SPI_CSX_DIS
#define SPI2_DMA_CONF_REG   (*((volatile uint32_t *)0x60024030)) //SPI_CSX_DIS
#define SPI2_W0_REG         (*((volatile uint32_t *)0x60024098)) 

//GDMA controller base address 0x6003F000
//Page 367 TRM provides explaination on transmit channel and GDMA receive channel
#define GDMA_OUT_CONF0_CH0_REG (*((volatile uint32_t *)0x6003F060)) //GDMA_OUT_RST_CH0 is bit 0
#define GDMA_OUT_LINK_CH0_REG (*((volatile uint32_t *)0x6003F080)) //GDMA_OUTLINK_ADDR_CH0 is bit 0 to 19
#define GDMA_OUT_PERI_SEL_CH0_REG (*((volatile uint32_t *)0x6003F0A8)) //GDMA_PERI_OUT_SEL_CH0 is bit 0 to 5

#define GDMA_OUT_INT_ENA_CH0_REG  (*((volatile uint32_t *)0x6003F070)) //Used for enabling GDMA done transfer flag
#define GDMA_OUT_INT_ST_CH0_REG   (*((volatile uint32_t *)0x6003F06C)) //Used for reading GDMA done transfer flag
#define GDMA_OUT_INT_CLR_CH0_REG  (*((volatile uint32_t *)0x6003F074)) //Used for clearing GDMA done transfer flag
#define GDMA_OUT_INT_RAW_CH0_REG  (*((volatile uint32_t *)0x6003F068))

//#define GMDA_OUT_TOTAL_EOF_CH0 //Interrupt to wait for  to indicate complete data transfer

#define SYSTEM_PERIP_CLK_EN0_REG (*((volatile uint32_t *)0x600C0018))
#define SYSTEM_PERIP_RST_EN0_REG (*((volatile uint32_t *)0x600C0020))
#define SYSTEM_SPI2_CLK_EN (1 << 6)
#define SYSTEM_PERIP_CLK_EN1_REG (*((volatile uint32_t *)0x600C001C))
#define SYSTEM_PERIP_RST_EN1_REG (*((volatile uint32_t *)0x600C0024))
#define SYSTEM_DMA_CLK_EN (1 << 6)


// --- SPI clock: 40MHz = 80MHz / 2 ---
#define SPI_CLOCK_80MHZ  (1u<<31)  // SPI_CLK_EQU_SYSCLK = 1: SPI_CLK = APB = 80 MHz
#define SPI_CLOCK_40MHZ  (0u<<31) | (0u<<18) | (1u<<12) | (0u<<6) | (1u<<0)   //(STABLE)

// --- Pin assignments ---
#define GPIO_CS   10
#define GPIO_DIN  11
#define GPIO_CLK  12
#define GPIO_DC   13
#define GPIO_RST  9
#define GPIO_BL   14
#define GPIO_BTN_1 21
#define GPIO_BTN_2 1
#define GPIO_BTN_3 2
#define GPIO_BTN_4 4

// ----------------------------------------------------------------
// Pin helpers — match official driver's CS/DC manual control
// ----------------------------------------------------------------

#define CS_LOW()   GPIO_OUT_W1TC_REG = (1 << GPIO_CS)
#define CS_HIGH()  GPIO_OUT_W1TS_REG = (1 << GPIO_CS)
#define DC_LOW()   GPIO_OUT_W1TC_REG = (1 << GPIO_DC)
#define DC_HIGH()  GPIO_OUT_W1TS_REG = (1 << GPIO_DC)
#define RST_LOW()  GPIO_OUT_W1TC_REG = (1 << GPIO_RST)
#define RST_HIGH() GPIO_OUT_W1TS_REG = (1 << GPIO_RST)
#define BL_HIGH()  GPIO_OUT_W1TS_REG = (1 << GPIO_BL)



#define DMA_BYTES 3840UL
#define DMA_DW0_OWNER_FLAG (1u << 31)
#define DMA_DW0_EOF_FLAG (1u << 30)
#define DMA_DW0_LENGTH_FLAG ((DMA_BYTES & 0xFFF) << 12)
#define DMA_DW0_SIZE_FLAG (DMA_BYTES << 0)

#define DMA_DW0 (DMA_DW0_OWNER_FLAG  | DMA_DW0_LENGTH_FLAG | DMA_DW0_SIZE_FLAG)
#define DMA_DW0_TAIL (DMA_DW0_OWNER_FLAG | DMA_DW0_EOF_FLAG | DMA_DW0_LENGTH_FLAG | DMA_DW0_SIZE_FLAG)

void ADC_Init();
int Read_Joystick_X();
int Read_Joystick_Y();
void SPI_Init();
void DMA_Init();
void Button_Init();
