/*Receptor para el Proyecto Final*/

#include <msp430.h>				
#include <msp430g2955.h>
#include <stdlib.h>
#include <stdint.h>
#include "diskio.h"
#include "integer.h"
#include "ff.h"
#include "uart.h"
#include "ctes_calibracion.h"
#include "xprintf.h"

#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif

typedef unsigned char bool_t;
FATFS fatfs;
bool_t fatfsOk = FALSE;

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
//
//void FF_init ()
//{
//    uint8_t res, max_tries;
//    FRESULT fres;
//
//    // Initialize FAT filesystem on MMC/SD card
//    max_tries = 5;
//    do
//    {   
//        send_uart("Inicializando FF_init\n\r");
//        res = disk_initialize (0);
//        send_uart("Fin Inicializacion\n\r");
//        __delay_cycles(100000); // wait 100 ms
//    } while ((res & STA_NOINIT) && max_tries-- > 0);
//    if (!(res & STA_NOINIT) && max_tries)
//    {
//        // FAT filesystem is OK
//        send_uart("DISK_init OK\n\r");
//        // Mounting filesystem
//        if ((fres = f_mount(0, &fatfs)) == FR_OK)
//        {
//            send_uart("Mount OK\n\r");
//            fatfsOk = TRUE;
//        }
//        else
//        {
//            send_uart("Mount ERR:\n\r");
//        }
//    }
//    else
//    {
//        send_uart("Disk_init ERR:2\n\r");
//    }
//    __delay_cycles(500000);
//}
#define SELECT()    P3OUT &= ~BIT0 
#define DESELECT()  P3OUT |= BIT0   // Card Deselect

int main(void) {
    WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer
    
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
    UCA0MCTL = UCBRS0; // Modulation UCBRSx = 1
    UCA0CTL1 &= ~UCSWRST; // **Initialize USCI state machine**
    IE2 |= UCA0RXIE; // Enable USCI_A0 RX interrupt
    /*FIN UART INIT*/
    
     /* Configure ports on MSP430 device for USCI_B */
    P3SEL |= BIT1 | BIT2 | BIT3;
    P3SEL2 &= ~(BIT1 | BIT2 | BIT3);
    UCB0CTL1 |= UCSWRST; /* USCI-B specific SPI setup */
    UCB0CTL0 = UCCKPH | UCMSB | UCMST | UCMODE_0 | UCSYNC; // SPI mode 0, master
    UCB0BR0 = 0x28; // 400kHz
    UCB0BR1 = 0x00;
    UCB0CTL1 = UCSSEL_2; // Clock = SMCLK, clear UCSWRST and enables USCI_B module.
    P3DIR |= BIT0; // P3.0 output direction
    
    
    __delay_cycles(16000000);
    xdev_out(&send_dato);
    P4DIR |= BIT3 | BIT4;
    P4OUT &= ~(BIT3 | BIT4);
    P4OUT |= BIT3;
    
    xputs("Iniciando\n\r");
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
    
    xprintf("%02x",res);
    
    do{
        res = xchg_spi(0xFF);
    } while ((res != 0xFF));
    
    xprintf("%02x",0xEE);
    
    P4OUT |= BIT4;
  
    DESELECT();
  
    FIL fsrc, fdst;      /* File objects */
    FRESULT fr;          /* FatFs function common result code */
    
    char str[12];

    /* Get volume label of the default drive */
    
    //disk_initialize(0);
    BYTE buff[512];
    FRESULT re;
    DWORD volid;
    //disk_read(0,buff,0,1);
    volid = LD_DWORD(buff[39]);
    //xprintf("volid %08x \n\r",volid);
    char buffer[] = "bolu";
    char recepbuff[20]="NADAESCRITOBOBO123";
    xputs("---------------\n\r");
    if((fr=f_mount(&fatfs, "0:", 1)) == FR_OK){
        P4OUT &= ~BIT4;
        xputs("\n\r------------------\n\r");
        FIL fdst, fsrc;
        //fr = f_getlabel("", str, 0);
//        fr = f_open(&fsrc, "0:docu.txt", FA_OPEN_EXISTING | FA_READ);
//        if (fr) {xprintf("f_open_read %02x",(int)fr); return (int)fr;}
        UINT wc=0;
//        fr = f_read(&fsrc, recepbuff, 16, &wc);
//        xprintf("Leido\n\r%s\n\r",recepbuff);
//        f_close(&fsrc);

//        fr = f_open(&fdst, "0:escri.txt", FA_CREATE_ALWAYS | FA_WRITE);
//        if (fr) {xprintf("FA_CREATE_ALWAYS f_open %02x",(int)fr); return (int)fr;}
//        f_close(&fdst);
//        f_mount(NULL, "0:", 0);
//        if(f_mount(&fatfs, "0:", 1)){xputs("2mono"); return 1;}
        wc=0;
        xputs("\n\r-----------CrearDocumento--------------\n\r");
        fr = f_open(&fdst, "0:docu3.txt", FA_WRITE | FA_OPEN_ALWAYS);
        if (fr) {xprintf("FA_OPEN_ALWAYS f_open %02x",(int)fr); return (int)fr;}
        fr = f_lseek(&fdst, f_size(&fdst));
        fr = f_write(&fdst, "\n\r Igual que yo!!!",18, &wc);

        if (fr) {xprintf("f_write %02x",(int)fr); return (int)fr;}
        xprintf("\n\rEscritos %02x",wc);
        fr=f_close(&fdst);
        xprintf("\n\rCerrado result: %02x",(int)fr);
        xputs("\n\r-----------------------------------------\n\r");
        //        fr = f_open(&fdst, "0:docu.txt", FA_OPEN_EXISTING | FA_WRITE);
//                if (fr) {xprintf("FA_OPEN_EXISTING f_open %02x",(int)fr); return (int)fr;}
//                fr = f_write(&fdst, "Lagarto2", 8, &wc);
//                if (fr) {xprintf("f_write %02x",(int)fr); return (int)fr;}
//                f_close(&fdst);
//

        f_mount(NULL, "0:", 0);

        xprintf("\n\rFin %02x,",(int)fr);
        xprintf("\n\rEscritos/Leidos %02x",(int)wc);
    }else{
        
        xprintf("No se pudo montar la tarjeta ERROR: %02x\n\r",fr);
        return 1;
    }
    
    while (1);
}
