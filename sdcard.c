#include <xc.h>
#include "sdcard.h"
#include "spi.h"
#include "uart.h"

// 512 byte read buffer
//unsigned char SDRdata[512];
// 512 byte write buffer
//unsigned char SDWdata[512];
// 512 byte block buffer
unsigned char SDdata[512];
// SD command buffer
unsigned char SDcommand[6];
// no response flag
unsigned char no_response = 0;
// holds SD card response byte
unsigned char card_response = 0;
// timeout variable to determine card timeout
unsigned int timeout = SD_TIMEOUT;

void SDcard_init(void){
    // send debug message
    uart_puts("Starting SD card initialization...\n");
    // configure CS pin
    CSTRIS &= ~CS; // configure CS as output
    CS_high; // set CS high

    // send 80 clocks (10 bytes) to wake up SD card
    // load dummy values into buffer
    for(unsigned char i = 0; i < 10; i++){
	SDdata[i] = 0xFF;
    }
    spi_send(SDdata, 10);

    // set CS low
    CS_low;

    uart_puts("Sending CMD0, awaiting response...");
    // transmit command 0
    SDcommand[0] = 0x40; // start bit | host bit | command
    SDcommand[1] = 0x00; // no arguments
    SDcommand[2] = 0x00;
    SDcommand[3] = 0x00;
    SDcommand[4] = 0x00;
    SDcommand[5] = 0x95; // precalculated CRC
    spi_send(SDcommand, 6);

    // read back response
    SDcard_get_response(0x01);
    uart_puts("success!\n");

    uart_puts("Sending CMD1, awaiting response...");
    // load command 1
    SDcommand[0] = 0x41; // start bit | host bit | command
    SDcommand[1] = 0x00; // no arguments
    SDcommand[2] = 0x00;
    SDcommand[3] = 0x00;
    SDcommand[4] = 0x00;
    SDcommand[5] = 0x95; // CRC not needed, dummy byte
    // wait for SD card to go to idle mode
    no_response = 1;
    while(no_response){
            // send command 1
            spi_send(SDcommand, 6);
            // read back response
            // response time is 0 to 8 bytes
            spi_receive(SDdata, 8);
            for(unsigned char b = 0; b < 7; b++){
                    if(SDdata[b] == 0x00) no_response = 0;
            }
    }
    uart_puts("success!\n");
    // set SD card CS high
    CS_high;
    uart_puts("SD card initialized successfully!\n");
}

void SDcard_read_block(unsigned long address){
    uart_puts("Sending CMD17...");
    // set CS low
    CS_low;
    // load CMD17 with proper
    // block address
    SDcommand[0] = 0x51; // 0x40 | 0x11 (17)
    SDcommand[1] = (address>>24) & 0xFF;
    SDcommand[2] = (address>>16) & 0xFF;
    SDcommand[3] = (address>>8) & 0xFF;
    SDcommand[4] = address & 0xFF;
    SDcommand[5] = 0xFF;

    // send command
    spi_send(SDcommand, 6);

    // read back response
    SDcard_get_response(0x00);
    uart_puts("success!\n");

    // read back response and
    // wait for data token FEh
    SDcard_get_response(0xFE);
    // receive data block
    spi_receive(SDdata, 512);
    // flush two bytes of CRC data
    spi_single_send(0xFF);
    spi_single_send(0xFF);
    spi_single_send(0xFF);

    // set SD card CS high
    CS_high;
}

void SDcard_write_block(unsigned long address){
    uart_puts("Sending CMD24...");
    // set CS low
    CS_low;
    // load CMD24 with proper
    // block address
    SDcommand[0] = 0x58; // 0x40 | 0x18 (24)
    SDcommand[1] = (address>>24) & 0xFF;
    SDcommand[2] = (address>>16) & 0xFF;
    SDcommand[3] = (address>>8) & 0xFF;
    SDcommand[4] = address & 0xFF;
    SDcommand[5] = 0xFF;

    // send command
    spi_send(SDcommand, 6);

    // read back response
    SDcard_get_response(0x00);
    uart_puts("success!\n");

    // send a one byte gap
    spi_single_send(0xFF);
    
    uart_puts("Sending data block...");
    // begin block write
    // send data token 0xFE
    spi_single_send(0xFE);
    // write data block
    spi_send(SDdata, 512);
    // send two byte CRC data
    spi_single_send(0xFF);
    spi_single_send(0xFF);

    // read data response
    card_response = spi_single_receive();

    if( (card_response & 0x0F) == 0x0D ) uart_puts("write error occured!\n");
    if( (card_response & 0x0F) == 0x0B ) uart_puts("CRC error occured!\n");
    if( (card_response & 0x0F) == 0x05 ){
        uart_puts("success!\n");
        uart_puts("Wating for card to finish write process...");
        // wait for card to be ready
        if(SDcard_get_response(0xFF)) uart_puts("timeout!");
        uart_puts("done!\n");
    }
    
    // set SD card CS high
    CS_high;
}

unsigned char SDcard_get_response(unsigned char response){
    // read back response
    // response time is 0 to 8 bytes
    no_response = 1;
    while(no_response && timeout){
        if(spi_single_receive() == response) no_response = 0; // check if response matches
        timeout--;
    }
    if(timeout == 0){ // if loop has timed out
        return 1;
    }
    // if response received
    return 0;
}