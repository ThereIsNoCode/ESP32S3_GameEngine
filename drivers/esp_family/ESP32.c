// #include "ESP32.h"

// #define DPORT_SPI_DMA_CLK_EN (1 << 22)

// //IO_MUX_X_REG configutation
// #define FUN_IE   (1u << 9)
// #define FUN_WPU  (1u << 8)
// #define GPIO_FUNC (2u << 12)

// void Button_Init(){
    
//     // RTCIO_TOUCH_PAD7_REG |= (1u << 11);    // TO_GPIO — route pad to digital
//     // RTCIO_TOUCH_PAD7_REG |= (1u << 27);    // RUE — RTC pull-up ON (this is the missing piece)
//     // RTCIO_TOUCH_PAD7_REG &= ~(1u << 28);   // RDE — RTC pull-down OFF
//     IO_MUX_GPIO21_REG =  GPIO_FUNC | FUN_IE | FUN_WPU;   // GPIO13 → GPIO function (MCU_SEL=2)
//     IO_MUX_GPIO22_REG =  GPIO_FUNC | FUN_IE | FUN_WPU;  // GPIO14 → GPIO function
//     IO_MUX_GPIO19_REG =  (2u << 12) | FUN_IE | FUN_WPU;
//     // ensure not outputs
//     GPIO_ENABLE_W1TC_REG = (1u<<21) | (1u<<22) | (1u << 19);
// }

// void SPI_Init(){
//     ESP_LOGI(DISPLAY_TAG, "Enabling SPI3 clock...");
//     DPORT_PERIP_CLK_EN_REG |= DPORT_SPI3_CLK_EN | DPORT_SPI_DMA_CLK_EN;
//     DPORT_PERIP_RST_EN_REG |= DPORT_SPI3_CLK_EN | DPORT_SPI_DMA_CLK_EN;    // assert reset
//     DPORT_PERIP_RST_EN_REG &= ~(DPORT_SPI3_CLK_EN | DPORT_SPI_DMA_CLK_EN);   // release reset

//     ESP_LOGI(DISPLAY_TAG, "Resetting SPI3 registers...");
//     SPI3_CMD_REG   = 0;
//     SPI3_USER_REG  = 0;
//     SPI3_USER1_REG = 0;
//     SPI3_CTRL_REG  = 0;
//     SPI3_CTRL2_REG = 0;
//     SPI3_CLOCK_REG = 0;
//     SPI3_PIN_REG   = 0;

//     ESP_LOGI(DISPLAY_TAG, "Configuring IO_MUX...");
//     IO_MUX_GPIO5_REG  = IO_MUX_OUT;   // CS
//     IO_MUX_GPIO23_REG = IO_MUX_OUT;   // DIN (MOSI)
//     IO_MUX_GPIO18_REG = IO_MUX_OUT;   // CLK
//     IO_MUX_GPIO17_REG = IO_MUX_OUT;   // DC
//     IO_MUX_GPIO16_REG = IO_MUX_OUT;   // RST
//     IO_MUX_GPIO4_REG  = IO_MUX_OUT;   // BL

//     ESP_LOGI(DISPLAY_TAG, "Routing GPIO matrix...");
//     GPIO_OUT_SEL(GPIO_CLK) = SIGNAL_NUM_VSPICLK;   // 63 → GPIO18
//     GPIO_OUT_SEL(GPIO_DIN) = SIGNAL_NUM_VSPID;     // 65 → GPIO23

//     ESP_LOGI(DISPLAY_TAG, "Enabling GPIO outputs...");
//     GPIO_ENABLE_REG |= (1 << GPIO_CLK) | (1 << GPIO_DIN) | (1 << GPIO_CS);
//     GPIO_ENABLE_REG |= (1 << GPIO_DC)  | (1 << GPIO_RST) | (1 << GPIO_BL);

//     ESP_LOGI(DISPLAY_TAG, "Configuring SPI peripheral...");
//     SPI3_CLOCK_REG = SPI_CLOCK_40MHZ;
//     SPI3_PIN_REG   = SPI_PIN_CS0_DIS | SPI_PIN_CS1_DIS | SPI_PIN_CS2_DIS;

//     //Setting up SPI_DMA
//     DPORT_SPI_DMA_CHAN_SEL_REG = 0b010000; //DMA Channel 2 for SPI3

//     //Force reset on DMA inbound and outbound states
//       //Force reset on DMA inbound and outbound states
//     SPI3_DMA_CONF_REG |=  (1<<2)|(1<<3)|(1<<4)|(1<<5);   // assert IN_RST, OUT_RST, AHBM_FIFO_RST, AHBM_RST
//     SPI3_DMA_CONF_REG &=  ~((1<<2)|(1<<3)|(1<<4)|(1<<5));   // deassert IN_RST, OUT_RST, AHBM_FIFO_RST, AHBM_RST
//     //Ensures 
//     //1. outlink Descripters are read in burst mode
//     //2. lets DMA read pixel data from memory in burst mode
//     SPI3_DMA_CONF_REG |= (1<<9)|(1<<12)|(1<<10);

//     SPI3_DMA_INT_ENA_REG |= (1u << 8);   // enable OUT_TOTAL_EOF_INT

//     CS_HIGH();
// }
