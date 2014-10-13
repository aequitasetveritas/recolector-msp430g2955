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
	unsigned int size = strlen(data);      //get length of data to be sent
	for (i = 0; i < size; i++) {
		while (!(IFG2 & UCA0TXIFG));      //Wait UART to finish before next send
		UCA0TXBUF = data[i];
	}
}

void send_dato(char data){
    while (!(IFG2 & UCA0TXIFG)); //Wait UART to finish before next send
    UCA0TXBUF = data;
}
