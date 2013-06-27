/* 
 * File:   spi.h
 * Author: Torrentula
 *
 * Created on 11. März 2013, 16:30
 */

#ifndef SPI_H
#define	SPI_H

#define CS 0x02
#define CSLAT LATC
#define CSTRIS TRISC

#define CS_low (CSLAT &= ~(CS))
#define CS_high (CSLAT |= CS)

void spi_init(unsigned char speed_mode);
void spi_send_byte(unsigned char data);
void spi_send(unsigned char* data, unsigned int length);
void spi_receive(unsigned char* data, unsigned int length);
void spi_single_send(unsigned char data);
unsigned char spi_single_receive(void);

#endif	/* SPI_H */

