/*Receptor para el Proyecto Final*/

#include <msp430.h>				
#include <msp430g2955.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "xprintf.h"
#include "diskio.h"
#include "integer.h"
#include "ff.h"
#include "uart.h"
#include "msprf24.h"
#include "ctes_calibracion.h"
#include "memFlash.h"
#include "nrf.h"

#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif

inline void crono_on();
inline uint32_t crono_off();
void recibir_datos(void);
uint8_t limpiar_evento(uint8_t * evento, uint8_t * linea);
void init_SD(void);
void tirar_al_archivo(void);

typedef unsigned char bool_t;
FATFS fatfs;
bool_t fatfsOk = FALSE;

volatile uint16_t timer_cont;

DWORD get_fattime() {
    DWORD tmr;

    tmr = (((DWORD) 2014 - 1980) << 25)
            | ((DWORD) 10 << 21)
            | ((DWORD) 2 << 16)
            | (WORD) (11 << 11)
            | (WORD) (15 << 5)
            | (WORD) (30 >> 1);

    return tmr;
}

#define ERROR_STR_NUMBER    13

void menu_sd(void);
const char *FF_getErrorStr(FRESULT fres) {
    static const char *errorStr[ERROR_STR_NUMBER] ={
        "FR_OK", /* 0 */
        "FR_NOT_READY", /* 1 */
        "FR_NO_FILE", /* 2 */
        "FR_NO_PATH", /* 3 */
        "FR_INVALID_NAME", /* 4 */
        "FR_INVALID_DRIVE", /* 5 */
        "FR_DENIED", /* 6 */
        "FR_EXIST", /* 7 */
        "FR_RW_ERROR", /* 8 */
        "FR_WRITE_PROTECTED", /* 9 */
        "FR_NOT_ENABLED", /* 10 */
        "FR_NO_FILESYSTEM", /* 11 */
        "FR_INVALID_OBJECT" /* 12 */
    };
    static const char unknownErrorStr[] = "Unknown error";
    if (fres >= 0 && fres <= ERROR_STR_NUMBER) {
        return errorStr[fres];
    } else {
        return unknownErrorStr;
    }
}

#define SELECT()    P3OUT &= ~BIT0 
#define DESELECT()  P3OUT |= BIT0   // Card Deselect

int main(void) {
    WDTCTL = WDTPW | WDTHOLD; // Detener watchdog timer
    
    grabar_cts_DCO_955();
    
    /*UART INIT*/
    DCOCTL = 0; // Select lowest DCOx and MODx settings
    BCSCTL1 = CALBC1_16MHZ; // Set DCO
    DCOCTL = CALDCO_16MHZ;
    BCSCTL2 |= DIVS_0; //SMCLK 16Mhz
    P3SEL |= (BIT4 | BIT5); // P1.1 = RXD, P1.2=TXD
    P3SEL2 &= ~(BIT4 | BIT5); // P1.1 = RXD, P1.2=TXD
    UCA0CTL1 |= UCSSEL_2; // SMCLK
    UCA0BR0 = 0x41; //  19200
    UCA0BR1 = 0x03; //  
    UCA0MCTL = UCBRS0; // Modulacion UCBRSx = 1
    UCA0CTL1 &= ~UCSWRST; // **Inicializar USCI**
    IE2 |= UCA0RXIE; // Habilitar la interrupcion USCI_A0 RX
    /*FIN UART INIT*/
    
     /* Configurar puertos en MSP430 device for USCI_B */
    P3SEL |= BIT1 | BIT2 | BIT3;
    P3SEL2 &= ~(BIT1 | BIT2 | BIT3);
    UCB0CTL1 |= UCSWRST; /* USCI-B specific SPI setup */
    UCB0CTL0 = UCCKPH | UCMSB | UCMST | UCMODE_0 | UCSYNC; // SPI mode 0, master
    UCB0BR0 = 0x28; // 16Mhz/0x28 = 400kHz
    UCB0BR1 = 0x00;
    UCB0CTL1 = UCSSEL_2; // Clock = SMCLK, clear UCSWRST and enables USCI_B module.
    P3DIR |= BIT0; // P3.0 output direction
    
    /* UART y SPI inicializados */
    
    __delay_cycles(16000000);
    
    xdev_out(&send_dato);	//Punteros a función para la comunicacion UART.
    xdev_in(&recibir_dato);

    P4DIR |= BIT3 | BIT4; //Leds
    P4OUT &= ~(BIT3 | BIT4);

    P2DIR &= ~BIT5;
    P2OUT &= ~BIT5;
    P2REN |= BIT5; // P2.5 con pulldown

    P2IE |= BIT5; //Interrupciones habilitadas para P2.5;

    xputs("\n\r-----------------------------------------------------------------\n\r");

    xputs("1 - Memoria Flash\n\r");
    xputs("2 - PING nrf24l01+ & Arduino\n\r");
    xputs("3 - nfr24l01+ enviar comandos\n\r");
    xputs("4 - nrf24l01+ enviar y recibir\n\r");
    xputs("5 - Test SD\n\r");
    xputs("6 - Recibir Datos y grabar en la SD\n\r");
    xputs("7 - Tirar datos en la SD\n\r");
    xputs("8 - Enviar a la PC datos simulados\n\r");
    xputs("\n\rIngrese num de comando y presione [ENTER]: ");
    

    char buff[2];

    if (xgets(buff, 2)) {
	    P4OUT |= BIT4;
    } else {
	    P4OUT &= ~BIT4;
    }
    //
    if(!(strcmp(buff,"5")))
    {
        menu_sd();
    }
    if(!(strcmp(buff,"6")))
    {
        recibir_datos();
    }
    if(!(strcmp(buff,"7")))
    {
        tirar_al_archivo();
    }
    if(!(strcmp(buff,"8")))
    {
    	env_datos_simulados();
     }
    if(!(strcmp(buff,"1"))){
        char in[7];
	    msprf24_init();
        xputs("Escribir o Leer: (e/l)\n\r");
	    char buffe[256];
	    //while(P4IN & BIT2); //polling 4.2
	    nrf_limpiar_flags();

	    int8_t i;
	    for(i=0; i<32 ;i++){
		    in[i] = 0x00;
	    }
	    xputs("\n\rMensaje: ");
	    xgets((char *)in, 31);
	    nrf_cargar_y_transmitir(in);
	    if (nrf_esperar() != 0x20) {
		    xputs("Error NO Transmision\n\r");
	    }else{
		    xputs("\n\rACK Recibido\n\r");
		    P4OUT ^= BIT4;
		    P4OUT ^= BIT3;
	    }


        char buffl[256];
	    int res = 1;
	    char dir[7];
	    uint32_t dire;
	    res = xgets(dir, 7);
	    if (!(strcmp(dir, "e"))) {
		    do{
			    xputs("Escribir 32 Bytes, direccion 6 digitos hexa:\n\r");
			    res = xgets(dir, 7);
			    dire = strtol(dir, NULL, 16);

			    xputs("¿Borrar el sector? (s/n):");
			    res = xgets(dir, 7);
			    if(!(strcmp(dir, "s"))){
				    write_enable();
				    xputs("Borrando el sector (4KB)\n\n");
				    sector_erase(dire);
				    __delay_cycles(16000000);
			    }

			    xputs("mensaje:\n\r");
			    res = xgets(buffe, 32);
			    write_enable();
			    page_program(dire, (uint8_t *)buffe, 32);
			    xputs("Programado. Continuar? (s/n)\n\r");
			    res = xgets(dir, 7);
		    }while(!(strcmp(dir,"s")));
	    }

        if (!(strcmp(dir, "l"))) {
            char in[7];
            //Leer una direccion de memoria;
            do {
			    xputs("\n\rLeer Pagina, direccion 6 digitos hexa: ");
                res = xgets(in, 7);
                dire = strtol(in, NULL, 16);
                xprintf("\n\rdire: %02x", (dire >> 16));
                xprintf("%04x\n\r",dire);
                read_data(dire, (uint8_t *)buffl, 256);
			    xputs("Lectura (256 Bytes):\n\r");
                xprintf("%s", buffl);
		    } while (res);
	    }


    }
    if(!(strcmp(buff,"2"))){
	    /* nrf24l01+ */
	    /* SPI ya esta inicializado */
	    P4OUT |= BIT4;
	    P4OUT &= ~BIT3;
	    xputs("Inicializando nrf24l01+:\n\r");
	    nrf_inicializar();
	    xputs("\n\rPreConf\n\r");
	    nrf_ver_registros();
	    xputs("\n\r");
	    nrf_conf_inicial();
	    xputs("\n\rPostConf\n\r");
	    nrf_ver_registros();
	    uint64_t txdir = 0x65646f4e32;
	    uint64_t rxdir = 0x65646f4e31;
	    uint8_t l_buff[32];
	    while(1){
		    //while(P4IN & BIT2); //polling 4.2

		    if(nrf_esperar() == 0x40){
			    // Se recibio un paquete.
			    nrf_limpiar_flags();
			    xputs("\n\rRecibido un paquete\n\r");
                nrf_leer_rx_payload(l_buff);
			    P4OUT ^= BIT4;
			    P4OUT ^= BIT3;

			    xprintf("\n\r%02x%02x%02x%02x\n\r",l_buff[3],l_buff[2],l_buff[1],l_buff[0]);

			    nrf_como_PTX(txdir);

			    nrf_cargar_y_transmitir(l_buff);
			    if(nrf_esperar() != 0x20){
				    xputs("Error NO Transmision\n\r");
			    }
			    nrf_como_PRX(rxdir);
		    }else{
			    xputs("Error\n\r");
			    nrf_limpiar_flags();
		    }
	    }
    }

    /* Enviar Comando */
    if(!(strcmp(buff,"3"))){
	    /* nrf24l01+ */
	    /* SPI ya esta inicializado */
	    P4OUT |= BIT4;
	    P4OUT &= ~BIT3;
	    xputs("----------------------------------");
	    xputs("Inicializando nrf24l01+:\n\r");
	    nrf_inicializar();
	    xputs("\n\rPreConf\n\r");
	    nrf_ver_registros();
	    xputs("\n\r");
	    nrf_limpiar_flags();
	    nrf_conf_inicial();
	    __delay_cycles(10000);

	    uint64_t txdir = 0x65646f4e31;
	    //uint64_t rxdir = 0x65646f4e31;
	    uint8_t in[32];

	    nrf_como_PTX(txdir);
	    xputs("\n\rPostConf\n\r");
	    nrf_ver_registros();
	    xputs("----------------------------------");

	    while (1) {
		    //while(P4IN & BIT2); //polling 4.2
		    nrf_limpiar_flags();

		    int8_t i;
		    for(i=0; i<32 ;i++){
			    in[i] = 0x00;
		    }
		    xputs("\n\rMensaje: ");
		    xgets((char *)in, 31);
		    nrf_cargar_y_transmitir(in);
		    if (nrf_esperar() != 0x20) {
			    xputs("Error NO Transmision\n\r");
		    }else{
			    xputs("\n\rACK Recibido\n\r");
			    P4OUT ^= BIT4;
			    P4OUT ^= BIT3;
		    }

	    }

    }

    // Enviar y Recibir
    if (!(strcmp(buff, "4"))) {
	    /* SPI ya esta inicializado */
	    P4OUT |= BIT4;
	    P4OUT &= ~BIT3;
	    xputs("----------------------------------");
	    xputs("Inicializando nrf24l01+:\n\r");
	    nrf_inicializar();
	    xputs("\n\rPreConf\n\r");
	    nrf_ver_registros();
	    xputs("\n\r");
	    nrf_limpiar_flags();
        nrf_conf_inicial();
        __delay_cycles(10000); // Delay para que todo se estabilice

	    uint64_t txdir = 0x65646f4e31;
	    uint64_t rxdir = 0x65646f4e32;
	    uint8_t in[32];
	    uint8_t in_num[5];
	    uint8_t cant_paq = 1;
	    uint8_t l_buff[33];

	    nrf_como_PTX(txdir);
	    xputs("\n\rPostConf\n\r");
	    nrf_ver_registros();
	    xputs("----------------------------------");

	    while (1) {
            nrf_limpiar_flags();
		    int8_t i;
		    for (i = 0; i < 32; i++) {
			    in[i] = 0x00;
		    }
		    xputs("\n\rMensaje: ");
		    xgets((char *) in, 31);

		    xputs("\n\rCantidad de paquetes a recibir: ");
		    xgets((char *) in_num, 4);
		    in_num[4] = 0x00;
            cant_paq = strtol((char *)in_num, NULL, 10);
            xprintf("\n\rEsperando %d paquetes\n\r",cant_paq);
		    nrf_cargar_y_transmitir(in);
		    if (nrf_esperar() != 0x20) {
			    xputs("Error NO Transmision\n\r");
		    } else {
			    nrf_como_PRX(rxdir);
			    xputs("\n\rACK Recibido, Esperando datos...\n\r");

			    while(cant_paq){
				    if(nrf_esperar()==0x40){
					    // Se recibio un paquete;
                        while(nrf_RX_pendientes() && cant_paq){
                            nrf_leer_rx_payload(l_buff);

                            //nrf_ver_registros();
                            nrf_limpiar_flags();
                            nrf_activar_RX(); // Se habilita la recepción de nuevo.
                            xputs("\n\rRecibido un paquete\n\r");
                            for(i=31;i>=0;i--){
                                xprintf("%02x",l_buff[i]);
                            }
                            l_buff[32] = 0x00;
                            xprintf("\n\r%s\n\r",l_buff);
                            cant_paq--;
                            P4OUT ^= BIT4;
                            P4OUT ^= BIT3;
                        }
				    }

			    }
		    }
		    nrf_como_PTX(txdir);
	    }
    }
}


void menu_sd(void){
    xputs("\n\r----------------------------------\n\r");
    xputs("1 - Crear Archivo\n\r");
    xputs("2 - Escribir en archivo\n\r");

    xputs("Iniciando\n\r");

    P4DIR |= BIT3 | BIT4;
    P4OUT &= ~(BIT3 | BIT4);
    P4OUT |= BIT3;

    xputs("Iniciando\n\r");
    /** Procedimiento para que la tarjeta ingrese en modo SPI: **/
    /** - SPI entre 100 y 400 MHZ
     * - Chip-Select en alto
     * - 74 o mas pulsos de reloj con MOSI en 1 (0xFF)
     * - Enviar el comando nativo CMD0 con Chip-Select en bajo
     * - Si la tarjeta ingresa en modo SPI, responde con R1 con
     *   el bit "In Idle State" en 1.
    **/
    DESELECT();
    int n;
    for (n = 100; n; n--) xchg_spi(0xFF);

    SELECT();
    uint8_t res;
    int k = 100;

    do{
        res = xchg_spi(0xFF);
    } while ((res != 0xFF) && k--);
    xprintf("%02x",res);
    xchg_spi(0x40); /* Command */
    xchg_spi(0x00); /* Argument[31..24] */
    xchg_spi(0x00); /* Argument[23..16] */
    xchg_spi(0x00); /* Argument[15..8] */
    xchg_spi(0x00); /* Argument[7..0] */
    xchg_spi(0x95);

    n = 10; /* Wait for a valid response in timeout of 10 attempts */
    do{
        res = xchg_spi(0xFF);
    }while ((res & 0x80));

    xprintf("Resp. Init SD: %02x\n\r",res);

    do{
        res = xchg_spi(0xFF);
    } while ((res != 0xFF));

    xputs("SD inicializada y lista\n\r");
    xputs("Cambiando Clock SPI a 8 Mhz\n\r");

    DESELECT();

    P4OUT |= BIT4;
    FIL fsrc, fdst;      /* File objects */
    FRESULT fr;          /* FatFs function common result code */

    char str[12];

    /* Get volume label of the default drive */

    //disk_initialize(0);

    BYTE buff[512];
    FRESULT re;
    DWORD volid;

    volid = LD_DWORD(buff[39]);

    char buffer[] = "bolu";
    char recepbuff[20]="NADAESCRITOBOBO123";
    xputs("---------------\n\r");
   crono_on();
    if((fr=f_mount(&fatfs, "0:", 1)) == FR_OK){
        P4OUT &= ~BIT4;
        xputs("\n\r------------------\n\r");
        FIL fdst, fsrc;
        UINT wc=0;
        wc=0;
        xputs("\n\r-----------CrearDocumento--------------\n\r");
        fr = f_open(&fdst, "0:docu4.txt", FA_WRITE | FA_OPEN_ALWAYS);
        if (fr) {xprintf("FA_OPEN_ALWAYS f_open %02x",(int)fr); return (int)fr;}
        fr = f_lseek(&fdst, f_size(&fdst));
        uint16_t i;
        char super_buff[1024];
        for(i=0;i<1024;i++)
        {
        	super_buff[i]='p';
        }
        char linea[]="linea 1";
        for(i=0;i<1;i++){
            fr = f_write(&fdst, super_buff,1024, &wc);
            if((i % 10)==0){
                P4OUT ^= BIT3;
            }

        }
        uint32_t ciclos = crono_off();
        xprintf("0x%08LX ciclos; ",ciclos);
        uint32_t useg = ciclos / 16L;
        xprintf("%ld microSegs.\n\r",useg);
        if (fr) {xprintf("f_write %02x",(int)fr); return (int)fr;}
        xprintf("\n\rEscritos %02x",wc);
        fr=f_close(&fdst);
        xprintf("\n\rCerrado result: %02x",(int)fr);
        xputs("\n\r-----------------------------------------\n\r");
        f_mount(NULL, "0:", 0);
        xprintf("\n\rFin %02x,",(int)fr);
        xprintf("\n\rEscritos/Leidos %02x",(int)wc);
    }else{

        xprintf("No se pudo montar la tarjeta ERROR: %02x\n\r",fr);
        return 1;
    }

    while (1);

}

enum estado { descarga, continuar, esperar, fin};
void tirar_al_archivo(void){
    enum estado est = descarga;
    enum estado prev_est = descarga;

    uint8_t buff_trx[33];
    buff_trx[32] = 0; // El caracter 33 es siempre el final de cadena.
   // uint8_t buff_evento[192];
    uint8_t buff_linea[96];
   // uint8_t pbl = 0;
   // uint8_t pbe = 0; // Puntero de buffer_evento
   // uint8_t buff_archivo[1024];
   // uint16_t pba = 0; // Puntero de buffer_archivo

   // uint8_t x;
   // uint16_t y; // Contadores
    int16_t k;
    // Configuración inicial como PTX.

    uint64_t txdir = 0x65646f4e31;
    uint64_t rxdir = 0x65646f4e32;

    // SPI a 400kHz
    init_SD();

    FRESULT fr;
    FIL fdst;      /* File objects */
    UINT wc=0;

    if((fr=f_mount(&fatfs, "0:", 1)) != FR_OK){
        xputs("Hubo un error al montar la tarjeta\n\r");
        return 1;
    }
    P4OUT |= BIT4;
    P4OUT &= ~BIT3;

    nrf_inicializar();
    nrf_limpiar_flags();
    nrf_conf_inicial(); // El nrf24l01+ queda como PRX escuchando en 0x65646f4e31
    __delay_cycles(10000); // Delay para que todo se estabilice
    nrf_como_PTX(txdir);

    fr = f_open(&fdst, "0:datos.txt", FA_WRITE | FA_CREATE_ALWAYS);
    if (fr) {xprintf("FA_CREATE_ALWAYS f_open %02x",(int)fr); return (int)fr;}
    fr = f_puts("================================================================================\n\r",&fdst); //80 + 2
    fr = f_puts("==============================================================================================\n\r",&fdst); //96
    fr = f_puts("================================================================================\n\r",&fdst); //80 + 2

    strcpy(buff_trx,"DESCARGAR");
    P4OUT |= BIT3;
    while(1){
        switch(est){
        case descarga:
            if(prev_est == esperar){
                nrf_como_PTX(txdir);
            }
            if(prev_est != descarga){
                strcpy(buff_trx,"DESCARGAR");
            }
            nrf_cargar_y_transmitir(buff_trx);
            while((nrf_esperar() != 0x20)){
                nrf_limpiar_y_retransmitir();
            }
            // Se transmitió correctamente la "baliza"
            //xputs("DESCARGAR\n\r");
            est = continuar;
            prev_est = descarga;
            break;
        case continuar:
            if(prev_est != continuar){
                strcpy(buff_trx,"CONTINUAR");
            }
            if(prev_est == esperar){
                nrf_como_PTX(txdir);
            }
            nrf_cargar_y_transmitir(buff_trx);
            if((nrf_esperar() != 0x20)){
                est = descarga; // No llego el ack de "CONTINUAR"
            }else{
                est = esperar; // Llego el ack de "CONTINUAR"
            }
            xputs("CONTINUAR\n\r");
            prev_est = continuar;
            break;
        case esperar:
            P4OUT ^= BIT4; // Togglea led verde.
            if(prev_est != esperar){
                nrf_como_PRX(rxdir);
            }
            xputs("ESPERAR\n\r");

            if(nrf_esperar() == 0x40){
                // Se recibio un paquete de datos
                for(k=0;k<32;k++){
                    buff_trx[k]=0;
                }
                nrf_leer_rx_payload(buff_trx);
                nrf_limpiar_flags();
                if (!strcmp(buff_trx,"FIN")){
                    // FIN de la transmision
                    xputs("\n\rFIN\n\r");
                    //fr = f_lseek(&fdst, f_size(&fdst));
                    //xputs("l_seek\n\r");
                    //fr = f_write(&fdst, buff_trx,32, &wc);
                    fr=f_close(&fdst);
                    xprintf("\n\rCerrado result: %02x",(int)fr);
                    xputs("\n\r-----------------------------------------\n\r");
                    f_mount(NULL, "0:", 0);
                    // Cerrar el archivo;
                    P4OUT |= BIT4;
                    P4OUT &= ~BIT3;
                     est = fin;
                }else{
                    if(buff_trx[15]=='E' && buff_trx[16]=='='){

                        uint8_t z=0;
                        strcpy(buff_linea,"Fecha y hora de lectura: ");
                        strncat(buff_linea,buff_trx,14);
                        strcat(buff_linea,"\t\tEnergia [kWh]: ");
                        for(z=0;z<10;z++){
                            buff_linea[56+z]=buff_trx[17+z]; //66;
                        }
                        for(z=0;z<30;z++){
                            buff_linea[66+z]=' ';
                        }
                        buff_linea[94]='\n';
                        buff_linea[95]='\r';
                        fr = f_lseek(&fdst, 82);
                        fr = f_write(&fdst, buff_linea,96, &wc);
                        fr = f_lseek(&fdst, f_size(&fdst));
                        xputs(buff_linea);
                        est = continuar;
                    }else{
                        //fr = f_lseek(&fdst, f_size(&fdst));
                        buff_trx[32] = 0;
                        fr = f_puts(buff_trx,&fdst);
                        xputs(buff_trx);
                        for(k=0;k<32;k++){
                            buff_trx[k]=0;
                        }

                        est = continuar;
                    }

                }
            }else{
                // No se recibio nada en x milisegundos
                est = descarga;
            }

            prev_est = esperar;
            break;
        case fin:
            break;
        }
    }

}

void recibir_datos(void){

    enum estado est = descarga;
    enum estado prev_est = descarga;

    uint8_t buff_trx[32];
    uint8_t buff_evento[192];
    uint8_t buff_linea[96];
    uint8_t pbl = 0;
    uint8_t pbe = 0; // Puntero de buffer_evento
    uint8_t buff_archivo[1024];
    uint16_t pba = 0; // Puntero de buffer_archivo

    uint8_t x;
    uint16_t y; // Contadores
    // Configuración inicial como PTX.

    uint64_t txdir = 0x65646f4e31;
    uint64_t rxdir = 0x65646f4e32;

    // SPI a 400kHz
    init_SD();

    FRESULT fr;
    FIL fdst;      /* File objects */
    UINT wc=0;

    if((fr=f_mount(&fatfs, "0:", 1)) != FR_OK){
        xputs("Hubo un error al montar la tarjeta\n\r");
        return 1;
    }
    P4OUT |= BIT4;
    P4OUT &= ~BIT3;

    nrf_inicializar();
    nrf_limpiar_flags();
    nrf_conf_inicial(); // El nrf24l01+ queda como PRX escuchando en 0x65646f4e31
    __delay_cycles(10000); // Delay para que todo se estabilice
    nrf_como_PTX(txdir);

    fr = f_open(&fdst, "0:data123.txt", FA_WRITE | FA_CREATE_ALWAYS);
    if (fr) {xprintf("FA_CREATE_ALWAYS f_open %02x",(int)fr); return (int)fr;}
    fr = f_puts("================================================================================\n\r",&fdst); //80 + 2
    fr = f_puts("==============================================================================================\n\r",&fdst); //96
    fr = f_puts("================================================================================\n\r",&fdst); //80 + 2

    while(1){
        switch(est){
        case descarga:
            if(prev_est == esperar){
                nrf_como_PTX(txdir);
            }
            if(prev_est != descarga){
                strcpy(buff_trx,"DESCARGAR");
            }
            nrf_cargar_y_transmitir(buff_trx);
            while((nrf_esperar() != 0x20)){
                nrf_limpiar_y_retransmitir();
            }
            // Se transmitió correctamente la "baliza"
            xputs("DESCARGAR\n\r");
            est = continuar;
            prev_est = descarga;
            break;
        case continuar:
            if(prev_est != continuar){
                strcpy(buff_trx,"CONTINUAR");
            }
            if(prev_est == esperar){
                nrf_como_PTX(txdir);
            }
            nrf_cargar_y_transmitir(buff_trx);
            if((nrf_esperar() != 0x20)){
                est = descarga; // No llego el ack de "CONTINUAR"
            }else{
                est = esperar; // Llego el ack de "CONTINUAR"
            }
            xputs("CONTINUAR\n\r");
            prev_est = continuar;
            break;
        case esperar:
            if(prev_est != esperar){
                nrf_como_PRX(rxdir);
            }
            xputs("ESPERAR\n\r");
            if(nrf_esperar() == 0x40){
                // Se recibio un paquete de datos
                nrf_leer_rx_payload(buff_trx);
                nrf_limpiar_flags();

                if(buff_trx[15]=='E'){
                    // It's energy baby.
                    uint8_t z=0;
                    strcpy(buff_linea,"Fecha y hora de lectura: ");
                    strncat(buff_linea,buff_trx,14);
                    strcat(buff_linea,"\t\tEnergia [kWh]: ");
                    for(z=0;z<10;z++){
                        buff_linea[56+z]=buff_trx[17+z]; //66;
                    }
                    for(z=0;z<30;z++){
                        buff_linea[66+z]=' ';
                    }
                    buff_linea[94]='\n';
                    buff_linea[95]='\r';

                    fr = f_lseek(&fdst, 82);
                    fr = f_write(&fdst, buff_linea,96, &wc);
                    xprintf("Energia %02x\n\r",fr);
                    est = continuar;
                }

                if (!strcmp(buff_trx,"FIN")){
                    // FIN de la transmision
                    est = fin;
                    xputs("\n\rFIN\n\r");
                    fr = f_lseek(&fdst, f_size(&fdst));
                    xputs("l_seek\n\r");
                    fr = f_write(&fdst, buff_archivo,pba, &wc);
                    fr=f_close(&fdst);
                    xprintf("\n\rCerrado result: %02x",(int)fr);
                    xputs("\n\r-----------------------------------------\n\r");
                    f_mount(NULL, "0:", 0);
                    // Cerrar el archivo;

                }else if(buff_trx[15]!='E'){
                    // Hacer lo que hay que hacer
                    est = continuar;
                    // Construir en buff_evento
                    xputs("DATO \n\r");
                    put_dump(buff_trx,buff_trx,32,DW_CHAR);
                    xputs("\n\r");
                    for(x=0;x<32;x++){
                        buff_evento[pbe + x]=buff_trx[x];
                    }
                    pbe = pbe + 32;

                    if(pbe==192){
                        pbe = 0;
                        // buff_evento esta lleno

                        for(x=168;x<192;x++){
                            buff_evento[x]=0;
                        }

                        xprintf("\nevento lleno \n%s\n",buff_evento);
                        // ¿Hay pendientes en buffer_linea?
                        if((pbl != 96) && (pbl != 0)){
                            for(x=0;x<(96 - pbl);x++){
                                buff_archivo[x]=buff_linea[x + pbl];
                            }
                            pba +=x;
                            pbl = 0;
                        }

                        if(limpiar_evento(buff_evento,buff_linea)){
                         // Evento de energia
                            for(x=0;x<192;x++){
                                buff_evento[x] = 0; // Limpiar
                            }
                            pbe = 0;
                        }else{
                            for(x=0;x<192;x++){
                                buff_evento[x] = 0; // Limpiar
                            }
                            pbe = 0;

                            // añadir al buff_archivo
                            for(x=0;x<96;x++){
                                if((pba + x)<1024){
                                    buff_archivo[pba + x]=buff_linea[x];
                                }else{
                                    break;
                                }
                            }
                            pbl = x;
                            pba += x;
                            if(pba == 1024){
                                // buff_archivo esta lleno
                                //Grabar en sd
                                //buff_archivo[1023]=0;
                                xputs("\n\rbuff_archivo\n\r");
                                put_dump(buff_archivo,buff_archivo,1024,8);
                                xputs("\n");
                                fr = f_lseek(&fdst, f_size(&fdst));
                                xputs("l_seek\n\r");
                                fr = f_write(&fdst, buff_archivo,1024, &wc);

                                for(y=0;y<1024;y++){
                                    buff_archivo[y]=0; //limpiar
                                }
                                pba = 0;
                            }
                        }
                    }


                    // Limpiarlo
                    // Añadirlo a buff_archivo
                }

            }else{
                // No se recibio nada en x milisegundos
                est = descarga;
            }

            prev_est = esperar;
            break;
        case fin:
            break;
        }
    }

}

// Limpiar_evento devuelve 1 si el evento es en realidad el valor de energia.
uint8_t limpiar_evento(uint8_t * evento, uint8_t * linea){
    uint8_t z;
    if(evento[15]=='E'){
        // It's energy baby.
        strcpy(linea,"Fecha y hora de lectura: ");
        strncat(linea,evento,14);
        strcat(linea,"\t\tEnergia total: ");
        for(z=0;z<10;z++){
            linea[56+z]=evento[17+z]; //66;
        }
        for(z=0;z<30;z++){
            linea[66+z]=' ';
        }
        linea[94]='\n';
        linea[95]='\r';
        return 1;
    }
    /* Evento
     * YY/MM/DD-hh:mm:ss   FIN_SOBRECORRIENTE , E=xxxxxxx.xx kWh, P = xxxxx.12 W, FP = 1.123 IND, f = 12.1 Hz, Vrms = 123.1 V, Vp = 123.1 V, Inrms = 12.123 A, inp = 12.123 A\n\r
     */
    for(z=0;z<17;z++){
        linea[z]=evento[z];
    }
    linea[17] = ',';

    for(z=0;z<18;z++){ // Evento
        linea[18+z] = evento[20+z];
    }
    linea[36] = ',';
    for(z=0;z<10;z++){ // Energia
        linea[37+z] = evento[43+z];
    }
    linea[47]= ',';
    for(z=0;z<8;z++){ // Pot
        linea[48+z] = evento[63+z];
    }
    linea[56]=',';
    for(z=0;z<5;z++){ // FP
        linea[57+z] = evento[80+z];
    }
    linea[62]=evento[86];
    linea[63]=',';
    for(z=0;z<4;z++){ // f
        linea[64+z] = evento[95+z];
    }
    linea[68]=',';
    for(z=0;z<5;z++){ // vrms
        linea[69+z] = evento[111+z];
    }
    linea[74]=',';
    for(z=0;z<5;z++){ // vp
        linea[75+z] = evento[125+z];
    }
    linea[80]=',';
    for(z=0;z<6;z++){ // irms
        linea[81+z] = evento[142+z];
    }
    linea[87]=',';
    for(z=0;z<6;z++){ // ipico
        linea[88+z] = evento[158+z];
    }
    linea[94]='\n';
    linea[94]='\r';
    xprintf("\n\r\n\rLINEA \n\r%s\n\r",linea);
    return 0;
}

void init_SD(void){
    /** Procedimiento para que la tarjeta ingrese en modo SPI: **/
    /** - SPI entre 100 y 400 MHZ
     * - Chip-Select en alto
     * - 74 o mas pulsos de reloj con MOSI en 1 (0xFF)
     * - Enviar el comando nativo CMD0 con Chip-Select en bajo
     * - Si la tarjeta ingresa en modo SPI, responde con R1 con
     *   el bit "In Idle State" en 1.
    **/
    DESELECT();
    int n;
    for (n = 100; n; n--) xchg_spi(0xFF);

    SELECT();
    uint8_t res;
    int k = 100;

    do{
        res = xchg_spi(0xFF);
    } while ((res != 0xFF) && k--);
    xprintf("%02x",res);
    xchg_spi(0x40); /* Command */
    xchg_spi(0x00); /* Argument[31..24] */
    xchg_spi(0x00); /* Argument[23..16] */
    xchg_spi(0x00); /* Argument[15..8] */
    xchg_spi(0x00); /* Argument[7..0] */
    xchg_spi(0x95);

    n = 10; /* Wait for a valid response in timeout of 10 attempts */
    do{
        res = xchg_spi(0xFF);
    }while ((res & 0x80));

    xprintf("Resp. Init SD: %02x\n\r",res);

    do{
        res = xchg_spi(0xFF);
    } while ((res != 0xFF));

    xputs("SD inicializada y lista\n\r");
    xputs("Cambiando Clock SPI a 8 Mhz\n\r");

    DESELECT();
}

inline void crono_on(){
 timer_cont = 0;
 TACTL |= MC_0;   // Stop
 TACTL |= TACLR;  // Reset del TimerA

 TACTL = TAIE | TASSEL_2 | MC_2;                  // Habilitada la int, SMCLK, contmode
 _EINT();
}

inline uint32_t crono_off(){
 TACTL |= MC_0;
 uint16_t cuenta = TAR;
 return ((((uint32_t)timer_cont) << 16 ) | ( 0x0000FFFF) & (uint32_t)(cuenta) );
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0RX_VECTOR))) USCI0RX_ISR (void)
#else
#error Compiler not supported!
#endif
{
	char dummy = UCA0RXBUF;
	LPM3_EXIT;
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER0_A1_VECTOR
__interrupt void Timer_A(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_A1_VECTOR))) Timer_A (void)
#else
#error Compiler not supported!
#endif
{
    P4OUT ^= BIT3 | BIT4;
    TACTL &= ~TAIFG;
    timer_cont++;
}

    // Port 2
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT2_VECTOR
    __interrupt void Port_2(void)
#elif defined(__GNUC__)
    void __attribute__ ((interrupt(PORT2_VECTOR))) Port_2 (void)
#else
#error Compiler not supported!
#endif
{
    tirar_al_archivo();
    P2IFG &= ~BIT5;                           // P2.5 IFG = 0
}
