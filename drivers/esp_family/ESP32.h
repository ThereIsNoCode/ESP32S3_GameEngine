// #pragma once
// #include "ESP32_DMA.h"
// #include <freertos/FreeRTOS.h> //Maybe move if needed
// #include <freertos/task.h>
// #include <esp_log.h>


// static const char *DISPLAY_TAG = "DISPLAY";

// // --- GPIO registers ---  ESP32
// #define GPIO_OUT_W1TS_REG (*((volatile uint32_t *)0x3FF44008))
// #define GPIO_OUT_W1TC_REG (*((volatile uint32_t *)0x3FF4400C))
// #define GPIO_ENABLE_REG   (*((volatile uint32_t *)0x3FF44020))
// #define GPIO_ENABLE_W1TC_REG (*((volatile uint32_t *)0x3FF44028))
// #define GPIO_IN_REG       (*((volatile uint32_t *)0x3FF4403C))

// #define RTCIO_RTC_GPIO_ENABLE_W1TC_REG (*((volatile uint32_t *)0x3FF48414))
// #define RTCIO_TOUCH_PAD7_REG (*((volatile uint32_t*)0x3FF484B0))   // GPIO27 = TOUCH7

// //Keep in mind, Since the two pins are pulled up through IO_MUX,
// //It should go to zero (ACTIVE LOW)
// #define INPUT_LEFT (((GPIO_IN_REG>>21) & 1) == 0) 
// #define INPUT_RIGHT (((GPIO_IN_REG>>22) & 1) == 0)
// #define INPUT_JUMP (((GPIO_IN_REG>>19) & 1) == 0)

// // --- IO_MUX registers --- ESP32
// #define IO_MUX_GPIO5_REG   (*((volatile uint32_t*)0x3FF4906C))
// #define IO_MUX_GPIO18_REG  (*((volatile uint32_t*)0x3FF49070))
// #define IO_MUX_GPIO17_REG  (*((volatile uint32_t*)0x3FF49050))
// #define IO_MUX_GPIO16_REG  (*((volatile uint32_t*)0x3FF4904C))
// #define IO_MUX_GPIO4_REG   (*((volatile uint32_t*)0x3FF49048))
// #define IO_MUX_GPIO23_REG   (*((volatile uint32_t*)0x3FF4908C))


// #define IO_MUX_GPIO21_REG   (*((volatile uint32_t*)0x3FF4907C))
// #define IO_MUX_GPIO22_REG   (*((volatile uint32_t*)0x3FF49080))
// #define IO_MUX_GPIO27_REG   (*((volatile uint32_t*)0x3FF4902C))
// #define IO_MUX_GPIO19_REG  (*((volatile uint32_t*)0x3FF49074))
// // --- SPI3 (VSPI) registers ---
// #define SPI3_CMD_REG     (*((volatile uint32_t *)0x3FF65000))
// #define SPI3_CTRL_REG    (*((volatile uint32_t *)0x3FF65008))
// #define SPI3_CTRL2_REG   (*((volatile uint32_t *)0x3FF65014))
// #define SPI3_CLOCK_REG   (*((volatile uint32_t *)0x3FF65018))
// #define SPI3_USER_REG    (*((volatile uint32_t *)0x3FF6501C))
// #define SPI3_USER1_REG   (*((volatile uint32_t *)0x3FF65020))
// #define SPI3_PIN_REG     (*((volatile uint32_t *)0x3FF65034))
// #define SPI3_W0_REG      (*((volatile uint32_t *)0x3FF65080))
// #define SPI3_W_REG(n)    (*((volatile uint32_t *)(0x3FF65080 + (n) * 4)))

// // --- DPORT clock gating ---
// #define DPORT_PERIP_CLK_EN_REG  (*((volatile uint32_t *)0x3FF000C0))
// #define DPORT_PERIP_RST_EN_REG  (*((volatile uint32_t *)0x3FF000C4))
// #define DPORT_SPI3_CLK_EN       (1 << 16)

// // --- GPIO matrix ---
// #define GPIO_FUNC_OUT_SEL_BASE  0x3FF44530
// #define GPIO_OUT_SEL(gpio)      (*((volatile uint32_t *)(GPIO_FUNC_OUT_SEL_BASE + (gpio) * 4)))

// // --- VSPI signal numbers ---
// #define SIGNAL_NUM_VSPICLK  63
// #define SIGNAL_NUM_VSPID    65
// #define SIGNAL_NUM_VSPICS0  68

// // --- SPI clock: 40MHz = 80MHz / 2 ---
// //#define SPI_CLOCK_40MHZ  (1 <<31)
// #define SPI_CLOCK_40MHZ  0x00001001u   //(STABLE)
// // --- SPI pin config: disable CS1 and CS2, CS0 controlled manually ---
// #define SPI_PIN_CS0_DIS  (1 << 2)
// #define SPI_PIN_CS1_DIS  (1 << 3)
// #define SPI_PIN_CS2_DIS  (1 << 4)

// // --- IO_MUX function: route through GPIO matrix ---
// #define IO_MUX_OUT  (2 << 12)

// // --- Pin assignments ---
// #define GPIO_CS   5
// #define GPIO_DIN  23
// #define GPIO_CLK  18
// #define GPIO_DC   17
// #define GPIO_RST  16
// #define GPIO_BL   4
// #define GPIO_LEFT_BTN 13
// #define GPIO_RIGHT_BTN 14
// #define GPIO_JUMP_BTN 19

// // ----------------------------------------------------------------
// // Pin helpers — match official driver's CS/DC manual control
// // ----------------------------------------------------------------

// #define CS_LOW()   GPIO_OUT_W1TC_REG = (1 << GPIO_CS)
// #define CS_HIGH()  GPIO_OUT_W1TS_REG = (1 << GPIO_CS)
// #define DC_LOW()   GPIO_OUT_W1TC_REG = (1 << GPIO_DC)
// #define DC_HIGH()  GPIO_OUT_W1TS_REG = (1 << GPIO_DC)
// #define RST_LOW()  GPIO_OUT_W1TC_REG = (1 << GPIO_RST)
// #define RST_HIGH() GPIO_OUT_W1TS_REG = (1 << GPIO_RST)
// #define BL_HIGH()  GPIO_OUT_W1TS_REG = (1 << GPIO_BL)

// // ----------------------------------------------------------------
// // SPI — bare byte transfer, CS controlled by caller
// // ----------------------------------------------------------------
// #define SPI3_MOSI_DLEN_REG  (*((volatile uint32_t *)0x3FF65028))


// //SPI3 DMA for OUTPUT
// #define SPI3_DMA_CONF_REG  (*((volatile uint32_t *)0x3FF65100))//Configuration for SPI3's DMA controller
// #define SPI3_DMA_OUT_LINK_REG (*((volatile uint32_t *)0x3FF65104)) //Sets OutLink Address / Config
// #define SPI3_DMA_STATUS_REG (*((volatile uint32_t *)0x3FF6510C))   //Especially usefuul for debugging
// #define DPORT_SPI_DMA_CHAN_SEL_REG (*((volatile uint32_t *)0x3FF005A8)) 

// //SPI3 DMA for INPUT
// #define SPI3_DMA_IN_LINK_REG (*((volatile uint32_t *)0x3FF65108))
// #define SPI3_IN_ERR_EOF_DES_ADDR_REG (*((volatile uint32_t *)0x3FF65120))
// #define SPI3_IN_SUC_EOF_DES_ADDR_REG (*((volatile uint32_t *)0x3FF65124))

// #define DMA_BYTES 3840
// #define DMA_DW0_OWNER_FLAG (1u << 31)
// #define DMA_DW0_EOF_FLAG (1u << 30)
// #define DMA_DW0_LENGTH_FLAG (DMA_BYTES << 12)
// #define DMA_DW0_SIZE_FLAG (DMA_BYTES << 0)

// #define DMA_DW0 (DMA_DW0_OWNER_FLAG  | DMA_DW0_LENGTH_FLAG | DMA_DW0_SIZE_FLAG)
// #define DMA_DW0_TAIL (DMA_DW0_OWNER_FLAG | DMA_DW0_EOF_FLAG | DMA_DW0_LENGTH_FLAG | DMA_DW0_SIZE_FLAG)

// #define SPI3_DMA_INT_ENA_REG  (*((volatile uint32_t *)0x3FF65110))
// #define SPI3_DMA_INT_ST_REG (*((volatile uint32_t *)0x3FF65118))
// #define SPI3_DMA_INT_CLR_REG (*((volatile uint32_t *)0x3FF6511C))
// #define SPI3_DMA_INT_RAW_REG (*((volatile uint32_t *)0x3FF65114))
// void SPI_Init();
// void Button_Init();
