#include "ESP32-S3.h"
//Handles Analog to Digital 
#include "esp_adc/adc_oneshot.h"
#define DPORT_SPI_DMA_CLK_EN (1 << 22)

//IO_MUX_X_REG configutation
#define FUN_IE   (1u << 9)
#define FUN_WPU  (1u << 8)
#define GPIO_FUNC (2u << 12)
static adc_oneshot_unit_handle_t adc1;
void ADC_Init()
{
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id  = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    adc_oneshot_new_unit(&unit_cfg, &adc1);

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,       // full ~0-3.3V range
        .bitwidth = ADC_BITWIDTH_DEFAULT,  // 12-bit
    };
    adc_oneshot_config_channel(adc1, ADC_CHANNEL_5, &chan_cfg); // GPIO6
    adc_oneshot_config_channel(adc1, ADC_CHANNEL_6, &chan_cfg); // GPIO7
}

#define JOYSTICK_DEADZONE 80

int Read_Joystick_X(){
    int x = 0;
    adc_oneshot_read(adc1, ADC_CHANNEL_5, &x);   // GPIO6
    x -= 1960;
    if(x < JOYSTICK_DEADZONE && x > -JOYSTICK_DEADZONE){
        x = 0;
    }
    else if(x < -1024){
        x = -1024;
    }
    else if(x > 1024){
        x = 1024;
    }
    return x;
}

int Read_Joystick_Y(){
    int y = 0;
    adc_oneshot_read(adc1, ADC_CHANNEL_6, &y);   // GPIO6
    y -= 1960;
    if(y < JOYSTICK_DEADZONE && y > -JOYSTICK_DEADZONE){
        y = 0;
    }
    else if(y < -1024){
        y = -1024;
    }
    else if(y > 1024){
        y = 1024;
    }
    return y;
}

//Initialize the four buttons 
void Button_Init(){
    
    // RTCIO_TOUCH_PAD7_REG |= (1u << 11);    // TO_GPIO — route pad to digital
    // RTCIO_TOUCH_PAD7_REG |= (1u << 27);    // RUE — RTC pull-up ON (this is the missing piece)
    // RTCIO_TOUCH_PAD7_REG &= ~(1u << 28);   // RDE — RTC pull-down OFF
    //IO_MUX_GPIO21_REG =  GPIO_FUNC | FUN_IE | FUN_WPU;   // GPIO13 → GPIO function (MCU_SEL=2)
    //IO_MUX_GPIO22_REG =  GPIO_FUNC | FUN_IE | FUN_WPU;  // GPIO14 → GPIO function
    //IO_MUX_GPIO19_REG =  (2u << 12) | FUN_IE | FUN_WPU;
    IO_MUX_GPIO21_REG |= GPIO_FUNC | (1<<9) | (1<<8);
    IO_MUX_GPIO1_REG |= GPIO_FUNC | (1<<9) | (1<<8);
    IO_MUX_GPIO2_REG |= GPIO_FUNC | (1<<9) | (1<<8);
    IO_MUX_GPIO4_REG |= GPIO_FUNC | (1<<9) | (1<<8);
    // ensure not outputs
    GPIO_ENABLE_W1TC_REG = (1u<<21) | (1u<<1) | (1u << 2) | (1<<4);
}

//Initialize the Joystick and its press to click feature
void Joystick_Init(){

}

#define I2S_TX_CLKM_CONF_REG (*((volatile uint32_t*)0x6000F034))
#define I2S_TX_CONF_REG (*((volatile uint32_t*)0x6000F024))
#define I2S_TX_CONF1_REG (*((volatile uint32_t*)0x6000F02C))


//Initialize Speaker Driver
void I2S_Init(){
    //Set I2S transmission Clock
    I2S_TX_CLKM_CONF_REG |= ( 2 << 27); //Sets Sample Rate
    I2S_TX_CLKM_CONF_REG |= (1<<29)|(1<<26);

    IO_MUX_GPIO16_REG |= (1<<12);
    IO_MUX_GPIO17_REG |= (1<<12);
    IO_MUX_GPIO18_REG |= (1<<12);

    GPIO_OUT_SEL(16)  = 22;                // route I2S0O_BCK_out to it (BCLK)
    GPIO_ENABLE_W1TS_REG = (1u << 16);     // enable output drive
    GPIO_OUT_SEL(17)  = 24;                // I2S0O_WS_out to it (LRC)
    GPIO_ENABLE_W1TS_REG = (1u << 16);     // 3. enable output drive    
    GPIO_OUT_SEL(18)  = 25;                // 2. route I2S0O_BCK_out to it
    GPIO_ENABLE_W1TS_REG = (1u << 16);     // 3. enable output drive  


    //Setting I2S_TX_SLAVE_MOD = 0
    I2S_TX_CONF_REG &= ~(1<<3);
    
    //Setting Bitwidth (each sample is 16 bit, essentially resolution)
    I2S_TX_CONF1_REG |= ( 16<<13);

    //Triggering Update
    I2S_TX_CONF_REG |= (1<<8);

    //Mentioned in 28.7 I2Sn Reset
    //Note: I2Sn module clock must be configured first before the module and FIFO are reset.
    //Resetting TX unit and FIFO
    I2S_TX_CONF_REG |= (1<<0)|(1<<1);

    //Over here: setup interrupt so we can detect end of transmission


    //Configure DMA Outlink

}

#define FUN_DRV  (2u << 10)   // 2 = medium drive strength
#define FUN_DRV_3  (3u << 10)   // maximum drive strength
//Initialize the SPI display
void SPI_Init(){
    ESP_LOGI(DISPLAY_TAG, "ESP32_ INITIALIZATION COMMENCE");
    //enable SPI2 Clock
    SYSTEM_PERIP_CLK_EN0_REG |= SYSTEM_SPI2_CLK_EN;
    SYSTEM_PERIP_RST_EN0_REG |= SYSTEM_SPI2_CLK_EN;
    SYSTEM_PERIP_RST_EN0_REG &= ~SYSTEM_SPI2_CLK_EN;
    
    //enable DMA clock
    SYSTEM_PERIP_CLK_EN1_REG |= SYSTEM_DMA_CLK_EN;
    SYSTEM_PERIP_RST_EN1_REG |= SYSTEM_DMA_CLK_EN;
    SYSTEM_PERIP_RST_EN1_REG &= ~SYSTEM_DMA_CLK_EN;

    ESP_LOGI(DISPLAY_TAG, "Resetting SPI3 registers...");
    SPI2_CMD_REG   = 0;
    SPI2_USER_REG  = 0;
    SPI2_USER1_REG = 0;
    SPI2_CTRL_REG  = 0;
    SPI2_DMA_CONF_REG = 0;
    SPI2_CLOCK_REG = 0;
    SPI2_MISC_REG   = 0;
    SPI2_MS_DLEN_REG = 0;
    

    ESP_LOGI(DISPLAY_TAG, "Configuring IO_MUX...");

    //TRM page 1132
    
    //Selects Function of pad to Function 4

    IO_MUX_GPIO10_REG = (0 << 12) | FUN_DRV;   // CS
    IO_MUX_GPIO11_REG = (4 << 12) | FUN_DRV;   // DIN (MOSI)
    IO_MUX_GPIO12_REG = (4 << 12) | FUN_DRV;   // CLK
    IO_MUX_GPIO13_REG = (1 << 12) | FUN_DRV;   // DC
    IO_MUX_GPIO9_REG = (1 << 12);   // DC
    IO_MUX_GPIO14_REG = (1 << 12);   // BL
    // IO_MUX_GPIO16_REG = IO_MUX_OUT;   // RST
    // IO_MUX_GPIO4_REG  = IO_MUX_OUT;   // BL

    // ESP_LOGI(DISPLAY_TAG, "Enabling GPIO outputs...");
    // Add DC and BL:
    GPIO_ENABLE_REG |= (1 << GPIO_CS)  |
                    (1 << GPIO_RST) |
                    (1 << GPIO_DC)  |
                    (1 << GPIO_BL);

    RST_HIGH();
    ESP_LOGI(DISPLAY_TAG, "Configuring SPI peripheral...");

    
    SPI2_USER_REG |= (1<<27);

    //40MHZ
    SPI2_CLK_GATE_REG = (1u<<0)|(1u<<1)|(1u<<2);   // bit2=1 → PLL_F80M source
    SPI2_CLOCK_REG    = SPI_CLOCK_40MHZ; //ORIGINAL WORKING


    //=SPI2_CLOCK_REG    =  (SPI_CLOCK_40MHZ);
    // SPI3_PIN_REG   = SPI_PIN_CS0_DIS | SPI_PIN_CS1_DIS | SPI_PIN_CS2_DIS;

    //Setting up SPI_DMA
    // DPORT_SPI_DMA_CHAN_SEL_REG = 0b010000; //DMA Channel 2 for SPI3

    GDMA_OUT_INT_ENA_CH0_REG = (1<<3);
    

    CS_HIGH();
}

void DMA_Init(){
    SPI2_DMA_CONF_REG |=  (1<<31)|(1<<30)|(1<<29);   // assert SPI_DMA_AFIFO_RST, SPI_BUF_AFIFO_RST, SPI_RX_AFIFO_RST
    SPI2_DMA_CONF_REG &=  ~((1<<31)|(1<<30)|(1<<29));    // deassert SPI_DMA_AFIFO_RST, SPI_BUF_AFIFO_RST, SPI_RX_AFIFO_RST
    SPI2_DMA_CONF_REG |= (1<<28); //Enables SPI_DMA_TX_ENA
}
