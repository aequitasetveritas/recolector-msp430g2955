#include "uart.h"
#include <msp430.h> 
#include <msp430g2955.h>
#include <string.h>

void init_UART(){
	DCOCTL = 0;                               // Select lowest DCOx and MODx settings
	BCSCTL1 = CALBC1_1MHZ;                    // Set DCO
	DCOCTL = CALDCO_1MHZ;
	P3SEL |= (BIT4 | BIT5) ;                     // P1.1 = RXD, P1.2=TXD
	P3SEL2 &= ~(BIT4 | BIT5);                    // P1.1 = RXD, P1.2=TXD
	UCA0CTL1 |= UCSSEL_2;                     // SMCLK
	UCA0BR0 = 104;                            // 1MHz 9600
	UCA0BR1 = 0;                              // 1MHz 9600
	UCA0MCTL = UCBRS0;                        // Modulation UCBRSx = 1
	UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
	IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt
}

void send_uart(char *data)
{
	unsigned int i;
	unsigned int size = strlen(data);
	for (i = 0; i < size; i++) {
		while (!(IFG2 & UCA0TXIFG));
		UCA0TXBUF = data[i];
	}
}

void send_dato(char data){
    while (!(IFG2 & UCA0TXIFG)); //Esperar que se transmita para mandar el caracter
    UCA0TXBUF = data;
}

unsigned char recibir_dato(){
	__bis_SR_register(LPM3_bits | GIE); //Espero la interrupcion
	 IE2 &= ~UCA0RXIE;
	unsigned char dummy;
	dummy = UCA0RXBUF;

	if (dummy == 0x13){
		 IE2 |= UCA0RXIE;
		return dummy; //Enter termina el stream
	}else{
		 IE2 |= UCA0RXIE;
		return dummy;
	}
}
