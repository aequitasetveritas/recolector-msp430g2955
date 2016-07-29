/*
 * funcionalidades.c
 *
 *  Created on: Dec 3, 2014
 *      Author: escualo
 */

#include "funcionalidades.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "xprintf.h"

void env_datos_simulados(void){
	const char * a[8];
    a[0] = "E=1234567.12 kWh\n";
    a[1] = "P = 12345.12 W\n";
    a[2] = "FP = -1.123 IND\n";
    a[3] = "f = 12.1 Hz\n";
    a[4] = "Vrms = 123.3 V\n";
    a[5] = "INrms = 12.2 A\n";
    a[6] = "Vp = 123.1 V\n";
    a[7] = "INp = 12.1 A\n";

	uint8_t i;
    while(1){
        for(i=0;i<8;i++){
            xputs(a[i]);
        }
        __delay_cycles(16000000);
    }
}

void env_datos_reales(void){

}


