/*
 * nrf.c
 *
 *  Created on: Nov 1, 2014
 *      Author: LGO
 */

#include <stdint.h>
#include <msp430.h>
#include "nrf.h"
#include "spi_g2955.h"
#include "xprintf.h"

#define habilitar_nrf() P4OUT &= ~CSN // Pone en bajo el chip-select (CSN) y habilita la com. SPI con el nrf24l01+.
#define deshabilitar_nrf() P4OUT |= CSN // Pone en alto el chip-select (CSN) y deshabilita com. SPI con el nrf24l01+.
#define activar_nrf() P4OUT |= CE
#define desactivar_nrf() P4OUT &= ~CE
#define CE BIT1
#define CSN BIT0
#define IRQ BIT2

/* nrf_inicializar() configura los puertos y pines para el CSN y CE. */
void nrf_inicializar(void){
	spi_inicializar();
	P4DIR |= CE | CSN; // CE y CSN como salidas
	P4DIR &= ~IRQ; 	// IRC como entrada
	P4OUT &= ~CE; 	// CE en bajo => deshabilitado
	P4OUT |= CSN; 	// CSN en alto => deshabilitado
}

static uint8_t leer_reg_8bits(uint8_t reg){
	habilitar_nrf();
	spi_transferir(0x00 | reg);
	uint8_t dummy = spi_transferir(0xFF);
	deshabilitar_nrf();
	return dummy;
}

static uint64_t leer_reg_40bits(uint8_t reg){
	habilitar_nrf();
	spi_transferir(0x00 | reg);
	uint8_t LSByte = spi_transferir(0xFF);
	uint8_t byte1 = spi_transferir(0xFF);
	uint8_t byte2 = spi_transferir(0xFF);
	uint8_t byte3 = spi_transferir(0xFF);
	uint8_t MSByte = spi_transferir(0xFF);
	deshabilitar_nrf();
	return ((uint64_t) MSByte << 32 | (uint64_t) byte3 << 24 | (uint64_t) byte2 << 16 | (uint64_t) byte1 << 8 | (uint64_t) LSByte << 0);
}

static void escribir_reg_8bits(uint8_t reg, uint8_t data)
{
	habilitar_nrf();
	spi_transferir(0x20 | reg);
	spi_transferir(data);
	deshabilitar_nrf();
}


static void escribir_tx_payload(uint8_t * buff){
	habilitar_nrf();
	spi_transferir(0xA0);
	int8_t i;
	for(i=0; i < 32; i++){
		spi_transferir(buff[i]);
	}
	deshabilitar_nrf();
}

/* Escribir_reg_40bits se usa para escribir las direcciones (TX_ADDR) (RX_ADDR) */
static void escribir_reg_40bits(uint8_t reg, uint64_t data){
	habilitar_nrf();
	uint8_t * lsbyte = (uint8_t *)&data;
	spi_transferir(0x20 | reg);
	spi_transferir(lsbyte[0]);
	spi_transferir(lsbyte[1]);
	spi_transferir(lsbyte[2]);
	spi_transferir(lsbyte[3]);
	spi_transferir(lsbyte[4]);
	deshabilitar_nrf();
}

/* Borra todos los datos en la cola TX FIFO */
static void flush_tx(){
	desactivar_nrf();
	habilitar_nrf();
	spi_transferir(0xE1);	// FlushTX
    __delay_cycles(10); // Delay necesario para que funcione el flush
	deshabilitar_nrf();
}

/* Borra todos los datos en la cola RX FIFO */
static void flush_rx(){
    desactivar_nrf();
    habilitar_nrf();
    spi_transferir(0xE2);	// FlushRX
    __delay_cycles(10); // Delay necesario para que funcione el flush
    deshabilitar_nrf();
}

void nrf_leer_rx_payload(uint8_t * buff){
	habilitar_nrf();
	spi_transferir(0x61);
	uint8_t i;
	for(i=0; i < 32; i++){
		buff[i] = spi_transferir(0xFF);
	}
	deshabilitar_nrf();
}

/* nrf_conf_inicial configura los registros del nrf para que funcione como PRX. 
 * En esta funcion se establece la velocidad, potencia y canal que se usaran en
 * el resto del programa */
void nrf_conf_inicial(void){
	desactivar_nrf();
	escribir_reg_8bits(0x00,0x0C); //PRIM_RX = 0
	escribir_reg_8bits(0x05,120); // Canal 120
	escribir_reg_8bits(0x06,0x07); // 1Mbps

	escribir_reg_8bits(0x02,0x03); // Habilitar recepcion en pipe0 y pipe1 [EN_RXADDR]
	escribir_reg_8bits(0x01,0x03); // Habilitar AutoAck en pipe0 y pipe1 [EN_AA]

	escribir_reg_8bits(0x03,0x03); // Direcciones de 5 bytes [SETUP_AW]
	escribir_reg_8bits(0x11,0x20); // pipe0 recibe payloads con 32 bytes
	escribir_reg_8bits(0x12,0x20); // pipe1 recibe payloads con 32 bytes

	escribir_reg_40bits(0x0A,0x65646f4e31); // pipe0 RX_ADDR

	escribir_reg_8bits(0x07,0x00); // Limpiar IRQs => REEMPLAZAR POR nrf_limpiar_flags();

	escribir_reg_8bits(0x00,0x0f); // Seleccionar PRIM_RX EN_CRC PWR_UP CRCO [CONFIG]
	
	// Delay de 5 ms (6.1.7 Timing Information [Tpd2stby] pag. 24).
	__delay_cycles(80000); // a 16MHz
    flush_rx();
	flush_tx();
	activar_nrf();

	// El nrf24l01+ queda como PRX activo.
}

/* Configura el nrf24l01+ como transmisor primario (PTX) en la direccion "dir" con AutoAck*/
void nrf_como_PTX(uint64_t dir){
	desactivar_nrf();
	nrf_limpiar_flags();
	escribir_reg_8bits(0x00,0x0C); // [CONFIG] como PTX y POWER-DOWN
	escribir_reg_40bits(0x0A,dir); // pipe0 RX_ADDR
	escribir_reg_40bits(0x10,dir); // TX_ADDR
	escribir_reg_8bits(0x04,0xFF); // 4ms y 15 ret.
    //__delay_cycles(10000); //CHECK THIS
    flush_rx();
	flush_tx();
    //__delay_cycles(10000);

	escribir_reg_8bits(0x00,0x0C | 0x02); // Power-Up

	// Delay de 5 ms (6.1.7 Timing Information [Tpd2stby] pag. 24).
	__delay_cycles(80000); // a 16MHz
	// Delay de 130 us (6.1.7 Timing Information [Tstby2a] pag. 24).
	__delay_cycles(3200); // a 16MHz
}

void nrf_como_PRX(uint64_t dir){
	desactivar_nrf();
	nrf_limpiar_flags();//
	escribir_reg_40bits(0x0A,dir); // pipe0 RX_ADDR
	escribir_reg_8bits(0x00,0x0f); // Seleccionar PRIM_RX EN_CRC PWR_UP CRCO [CONFIG]
	activar_nrf();
}

/* nrf_cargar_y_transmitir(uint8_t * data32bytes) borra cualquier paquete
 * pendiente, carga los 32 bytes del buffer data32bytes y los transmite */
void nrf_cargar_y_transmitir(uint8_t * data32bytes){
	desactivar_nrf();
	flush_tx();
	escribir_tx_payload(data32bytes);
	activar_nrf();
	__delay_cycles(2000);
	desactivar_nrf();
}

void nrf_limpiar_y_retransmitir(void){
    desactivar_nrf();
    nrf_limpiar_flags();
    activar_nrf();
    __delay_cycles(2000);
    desactivar_nrf();
}



void nrf_limpiar_flags(){
	desactivar_nrf();
	habilitar_nrf();
	escribir_reg_8bits(0x07,0x70);
	deshabilitar_nrf();
}

/* Imprime por UART los registros del nrf24l01+. Usar con cuidado porque lee la RX_FIFO
 * y saca un dato de la cola */
void nrf_ver_registros(void){

	xprintf("\n\rSTATUS:\t0x%02x\t0b%08b\n\r",leer_reg_8bits(0x07),leer_reg_8bits(0x07));
	xprintf("CONFIG:\t0x%02x\t0b%08b\n\r",leer_reg_8bits(0x00),leer_reg_8bits(0x00));

	xprintf("RX_ADDR_P0:\t0x%04x",(uint16_t)(leer_reg_40bits(0x0A) >> 24));
	xprintf("%04x",(uint16_t)(leer_reg_40bits(0x0A) >> 8));
	xprintf("%02x\n\r",(uint8_t)(leer_reg_40bits(0x0A)));

	xprintf("RX_ADDR_P1:\t0x%04x",(uint16_t)(leer_reg_40bits(0x0B) >> 24));
	xprintf("%04x",(uint16_t)(leer_reg_40bits(0x0B) >> 8));
	xprintf("%02x\n\r",(uint8_t)(leer_reg_40bits(0x0B)));

	xprintf("RX_ADDR_P2-P5:\t0x%02x\t0x%02x\t0x%02x\t0x%02x\n\r"
		,leer_reg_8bits(0x0C),leer_reg_8bits(0x0D),leer_reg_8bits(0x0E),leer_reg_8bits(0x0F));

	xprintf("TX_ADDR:\t0x%04x",(uint16_t)(leer_reg_40bits(0x10) >> 24));
	xprintf("%04x",(uint16_t)(leer_reg_40bits(0x10) >> 8));
	xprintf("%02x\n\r",(uint8_t)(leer_reg_40bits(0x10)));

	xprintf("EN_AA:\t\t0x%02x\n\r",leer_reg_8bits(0x01));
	xprintf("EN_RXADDR:\t0x%02x\n\r",leer_reg_8bits(0x02));
	xprintf("SETUP_AW:\t0x%02x\n\r",leer_reg_8bits(0x03));
	xprintf("SETUP_RETR:\t0x%02x\n\r",leer_reg_8bits(0x04));
	xprintf("RF_CH:\t\t0x%02x\n\r",leer_reg_8bits(0x05));
	xprintf("RF_SETUP:\t0x%02x\t0b%08b\n\r",leer_reg_8bits(0x06),leer_reg_8bits(0x06));
	xprintf("OBSERVE_TX:\t0x%02x\n\r",leer_reg_8bits(0x08));
	xprintf("RPD:\t\t0x%02x\n\r",leer_reg_8bits(0x09));
	xprintf("RX_PW_P0:\t0x%02x\n\r",leer_reg_8bits(0x11));
	xprintf("RX_PW_P1:\t0x%02x\n\r",leer_reg_8bits(0x12));
	xprintf("RX_PW_P2:\t0x%02x\n\r",leer_reg_8bits(0x13));
	xprintf("RX_PW_P3:\t0x%02x\n\r",leer_reg_8bits(0x14));
	xprintf("RX_PW_P4:\t0x%02x\n\r",leer_reg_8bits(0x15));
	xprintf("RX_PW_P5:\t0x%02x\n\r",leer_reg_8bits(0x16));
	xprintf("FIFO_STATUS:\t0x%02x\t0b%08b\n\r",leer_reg_8bits(0x17),leer_reg_8bits(0x17));
	xprintf("DYNPD:\t\t0x%02x\n\r",leer_reg_8bits(0x1C));
	xprintf("FEATURE:\t0x%02x\n\r",leer_reg_8bits(0x1D));
	int8_t i;
	uint8_t rbuff[32];
    nrf_leer_rx_payload(rbuff);
	xputs("RX_PAYLOAD: \t");
	for(i=31;i>=0;i--){
		xprintf("%02x",rbuff[i]);
	}
	xputs("\n\r");

}

/* nrf_esperar_respuesta() encuesta al pin P4.2 para saber cuando ocurrio algun evento */
inline uint8_t nrf_esperar(){
    //xputs("Esperando...\n\r");
	while(P4IN & IRQ); // Polling 4.2
	uint8_t respuesta;
	switch ( leer_reg_8bits(0x07) & 0x70 ) { // Leer status y filtrar los bits de interrupciones.
	case 0x40: // RX_DR = 1 Se recibio un paquete.
		respuesta = 0x40;
		break;
	case 0x20: // TX_DS = 1 Se transmitio exitosamente un paquete.
		respuesta = 0x20;
		break;
	case 0x10: // MAX_RT = 1 Se alcanzo el número máximo de retransmisiones.
		respuesta = 0x10;
		break;
	default:
		respuesta = 0x00;
		break;
	}
	//flush_tx();
	return respuesta;
}

/* nrf_RX_pendientes() devuelve 0 si la RX FIFO esta vacia
 * y 1 si existe algun paquete */
inline int8_t nrf_RX_pendientes(void){
    // Leer FIFO_STATUS, si RX_EMPTY esta seteado, RX FIFO esta vacia
    // y no hay pendientes.
    return (leer_reg_8bits(0x17) & 0x01) ? 0 : 1;
}

inline void nrf_activar_RX(void){
    activar_nrf();
}
