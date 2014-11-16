/*
 * spi_g2955.h
 *
 *  Created on: Oct 16, 2014
 *      Author: escualo
 */

#ifndef SPI_G2955_H_
#define SPI_G2955_H_

#include <stdint.h>


#define MEM_CS BIT0
#define MEM_HABILITAR_CS() P4OUT&=~MEM_CS
#define MEM_DESHABILITAR_CS() P4OUT|=MEM_CS

void spi_inicializar();
uint8_t spi_transferir(uint8_t);  // SPI xfer 1 byte
uint16_t spi_transferir16(uint16_t);  // SPI xfer 2 bytes

uint8_t spi_transferir_multi(uint8_t * datos, uint16_t n);
uint8_t spi_recibir_multi(uint8_t * datos, uint16_t n);

#endif /* SPI_G2955_H_ */
