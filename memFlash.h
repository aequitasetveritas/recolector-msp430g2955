#ifndef memFlash_H
#define memFlash_H
#include <stdint.h>

/*Todos las funciones remiten a una instrucción de la memoria*/

void write_enable();
void write_disable();
void page_program(uint32_t direccion, uint8_t * buff, uint16_t n);
void sector_erase(uint32_t direccion);
void block_erase32(uint32_t direccion);
void block_erase64(uint32_t direccion);
void chip_erase();
uint8_t read_status_register_1();
uint8_t read_status_register_2();
void read_data(uint32_t direccion, uint8_t * buff, uint16_t n);
uint8_t JedecID(uint8_t * buff);


#endif
