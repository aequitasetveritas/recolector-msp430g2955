#include "memFlash.h"
#include "spi_g2955.h"
#include <msp430.h>

static void instruccion_simple(uint8_t codigo);
static void instruccion_simple_dir(uint32_t direccion,uint8_t codigo);

void write_enable(){
	instruccion_simple(0x06);
}

void write_disable(){
	instruccion_simple(0x04);
}

void page_program(uint32_t direccion, uint8_t * buff, uint16_t n){
//	uint8_t * dir_byte = (uint8_t *) &direccion;

	uint8_t dir_byte[3];
	dir_byte[0] = (uint8_t)(direccion & 0x000000ff); 		//Byte Menos Significativo
	dir_byte[1] = (uint8_t)((direccion & 0x0000ff00) >> 8);
	dir_byte[2] = (uint8_t)((direccion & 0x00ff0000) >> 16); //Byte MAS Significativo

	MEM_HABILITAR_CS();
	spi_transferir(0x02);
	spi_transferir(dir_byte[2]);		//MSB al principio
	spi_transferir(dir_byte[1]);
	spi_transferir(dir_byte[0]);
	spi_transferir_multi(buff,n);
	MEM_DESHABILITAR_CS();
}

void sector_erase(uint32_t direccion){
	instruccion_simple_dir(direccion,0x20);
}

void block_erase32(uint32_t direccion){
	instruccion_simple_dir(direccion,0x52);
}

void block_erase64(uint32_t direccion){
	instruccion_simple_dir(direccion,0xD8);
}

void chip_erase(){
	instruccion_simple(0xC7);
}

uint8_t read_status_register_1(){
	MEM_HABILITAR_CS();
	uint8_t status = spi_transferir(0x05);
	MEM_DESHABILITAR_CS();
	return status;
}

uint8_t read_status_register_2(){
	MEM_HABILITAR_CS();
	uint8_t status = spi_transferir(0x35);
	MEM_DESHABILITAR_CS();
	return status;
}

void read_data(uint32_t direccion, uint8_t * buff, uint16_t n){
//	uint8_t * dir_byte = (uint8_t *) &direccion;
	uint8_t dir_byte[3];
	dir_byte[0] = (uint8_t)(direccion & 0x000000ff);
	dir_byte[1] = (uint8_t)((direccion & 0x0000ff00) >> 8);
	dir_byte[2] = (uint8_t)((direccion & 0x00ff0000) >> 16);

	MEM_HABILITAR_CS();
	spi_transferir(0x03);
	spi_transferir(dir_byte[2]);
	spi_transferir(dir_byte[1]);
	spi_transferir(dir_byte[0]);
	spi_recibir_multi(buff,n);
	MEM_DESHABILITAR_CS();
}

/*JedecID es una instrucción para identificar a la memoria segun
el estandar JEDEC. Devuelve 3 bytes: Codigo de fabricante, tipo de
memoria y capacidad. El código de fabricante para Windbond es 0xEF.*/

uint8_t JedecID(uint8_t * buff){
	MEM_HABILITAR_CS();
	spi_transferir(0x9F);  					//Codigo de instrucción 9F
	spi_recibir_multi(buff,3);
	MEM_DESHABILITAR_CS();

	if(buff[0]==0xFF){return 1;}			//Falla en la lectura
	return 0;
}

static void instruccion_simple(uint8_t codigo){
	MEM_HABILITAR_CS();
	spi_transferir(codigo);
	MEM_DESHABILITAR_CS();
}

static void instruccion_simple_dir(uint32_t direccion,uint8_t codigo){
//	uint8_t * dir_byte = (uint8_t *) &direccion;
	uint8_t dir_byte[3];
	dir_byte[0] = (uint8_t)(direccion & 0x000000ff);
	dir_byte[1] = (uint8_t)((direccion & 0x0000ff00) >> 8);
	dir_byte[2] = (uint8_t)((direccion & 0x00ff0000) >> 16);

	MEM_HABILITAR_CS();
	spi_transferir(codigo);
	spi_transferir(dir_byte[2]);
	spi_transferir(dir_byte[1]);
	spi_transferir(dir_byte[0]);
	MEM_DESHABILITAR_CS();
}
