/*
 * spi_g2955.c
 *
 *  Created on: Oct 16, 2014
 *      Author: escualo
 */

#include "spi_g2955.h"
#include <msp430.h>

void spi_inicializar(){
	P3SEL |= BIT1 | BIT2 | BIT3;
	P3SEL2 &= ~(BIT1 | BIT2 | BIT3);
	UCB0CTL1 |= UCSWRST; /* USCI-B SPI reset */
	UCB0CTL0 = UCCKPH | UCMSB | UCMST | UCMODE_0 | UCSYNC; // SPI mode 0, master
	UCB0BR0 = 0x28; // 400kHz
	UCB0BR1 = 0x00;
	UCB0CTL1 = UCSSEL_2; // Clock = SMCLK, limpiar UCSWRST y habilitar modulo USCI_B.
	P4DIR |= BIT0 | BIT1; // P3.0 output direction
}

uint8_t spi_transferir(uint8_t dat){
	uint8_t dummy;
	    while ((IFG2 & UCB0TXIFG) == 0); // Esperar a que se libere el buffer de transmisión
	    UCB0TXBUF = dat; // Escribir
	    while ((IFG2 & UCB0RXIFG) == 0); // Esperar a que se llene el buffer RX
	    dummy = UCB0RXBUF;
	    return (dummy);
}

uint16_t spi_transferir16(uint16_t dato16){
	//QUIZAS SEA NECESARIO CAMBIAR EL ORDEN
	uint8_t msb_res = spi_transferir((uint8_t) (dato16 >> 8));
	uint8_t lsb_res = spi_transferir((uint8_t) (dato16));

	uint16_t res = msb_res << 8 | lsb_res;
	return res;
}


uint8_t spi_transferir_multi(uint8_t * datos, uint16_t n){
	uint16_t i;
	uint8_t recibido = 0;

	for(i=0;i<n;i++){
		while(!(IFG2 & UCB0TXIFG));
		UCB0TXBUF = datos[i];
		while(!(IFG2 & UCB0RXIFG));
		recibido = UCB0RXBUF;
	}

	return recibido;
}

uint8_t spi_recibir_multi(uint8_t * datos, uint16_t n){
	uint16_t i;

	for(i=0;i<n;i++){
		while(!(IFG2 & UCB0TXIFG));
		UCB0TXBUF = 0xFF;
		while(!(IFG2 & UCB0RXIFG));
		datos[i] = UCB0RXBUF;
	}

	return datos[i];
}
