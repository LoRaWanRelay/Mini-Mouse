/*

  __  __ _       _                                 
 |  \/  (_)     (_)                                
 | \  / |_ _ __  _ _ __ ___   ___  _   _ ___  ___  
 | |\/| | | '_ \| | '_ ` _ \ / _ \| | | / __|/ _ \
 | |  | | | | | | | | | | | | (_) | |_| \__ \  __/ 
 |_|  |_|_|_| |_|_|_| |_| |_|\___/ \__,_|___/\___| 
                                                   
                                                   
Description       : User Define for loraMac Layer.  


License           : Revised BSD License, see LICENSE.TXT file include in the project

Maintainer        : Fabien Holin (SEMTECH)
*/
#ifndef USERDEFINE_H
#define USERDEFINE_H
#include "mbed.h"


/********************************************************************************/
/*                         Application     dependant                            */
/********************************************************************************/
#define DEBUG_TRACE    1      // set to 1 to activate debug traces
#define LOW_POWER_MODE 0      // set to 1 to activate sleep mode , set to 0 to replace by wait functions (easier in debug mode) 

#define SERIAL_TX       USBTX
#define SERIAL_RX       USBRX


/*SX126w BOARD specific */
//#define LORA_SPI_MOSI   PA_7
//#define LORA_SPI_MISO   PA_6
//#define LORA_SPI_SCLK   PA_5
//#define LORA_CS         PA_8
//#define LORA_RESET      PA_0
//#define TX_RX_IT        PB_4
//#define RX_TIMEOUT_IT   D14 // @TODO: Not required for 126x
//#define LORA_BUSY       PB_3

//#define CRYSTAL_ERROR       10 // for mcu clock with 3% accuracy (in case of clk is generated by internal rc for example)
//#define BOARD_DELAY_RX_SETTING_MS 7 // Time to configure board in rx mode

/*SX1276 BOARD specific */

#define LORA_SPI_MOSI       D11
#define LORA_SPI_MISO       D12
#define LORA_SPI_SCLK       D13
#define LORA_CS             D10
#define LORA_RESET          A0
#define TX_RX_IT            D2     // Interrupt TX/RX Done
#define RX_TIMEOUT_IT       D3     // Interrupt RX TIME OUT 
#define CRYSTAL_ERROR              56 // Crystal error of the MCU to fine adjust the rx window for lorawan ( ex: set 3� for a crystal error = 0.3%)
#define BOARD_DELAY_RX_SETTING_MS  4  // Delay introduce by the mcu Have to fine tune to adjust the window rx for lorawan
#define PA_BOOST_CONNECTED         1 //  Set to 1 to select Pa_boost outpin pin on the sx127x 

#define FLASH_UPDATE_PERIOD 1      // Lorawan store contaxt in flash with a period equal to FLASH_UPDATE_PERIOD transmit packets
#define USERFLASHADRESS 0x807F800U - 2048  // start flash adress to store lorawan context
#endif
