#include <p18LF14K50.h>
#include <i2c.h>
#include <usart.h>
#include <delays.h>
#include <stdlib.h>
#include <math.h>
#include "BMP085.h"

//#define I2C_SCL   TRISBbits.TRISB6
//#define I2C_SDA   TRISBbits.TRISB4
#define MULTIPLIER 100

void High_Int_Handler (void);   // Declaración rutina tratamiento interrupciones alta prioridad
void write8(unsigned char, unsigned char);
unsigned char read8(unsigned char);
unsigned int read16(unsigned char);
unsigned int readRawTemperature(void);
unsigned long readRawPressure(void);
float pow (auto float, auto float);
long readPressure(void);
float readAltitude(float );
float calcAltitude(float, long);

unsigned char num_dato, led_count;
unsigned char dato;
unsigned char oversampling;
int ac1, ac2, ac3, b1, b2, mb, mc, md;
unsigned int ac4, ac5, ac6;
long pressure;
char p[10];
float altitude;
char lalt[10], ralt[3];
long lWhole = 0;            // Store digits left of decimal	
unsigned long ulPart = 0;   // Store digits right of decimal
unsigned char mode=0;   // Oversampling mode

#pragma config FOSC = HS, FCMEN = OFF, IESO = OFF   // OSC=INTIO67 si osc int. OSC=HS si osc ext. 
#pragma config PWRTEN = OFF//, BOREN = SBORDIS//, BORV = ON	// BORV=0 si PIC18F4520; BORV=3 si PIC18LF4520
#pragma config WDTEN = OFF, WDTPS = 1
#pragma config MCLRE = ON
#pragma config STVREN = ON, LVP = OFF, XINST = OFF
#pragma config CP0 = OFF, CP1 = OFF
#pragma config CPB = OFF, CPD = OFF
#pragma config WRT0 = OFF, WRT1 = OFF
#pragma config WRTB = OFF, WRTC = OFF, WRTD = OFF
#pragma config EBTR0 = OFF, EBTR1 = OFF
#pragma config EBTRB = OFF

#pragma code High_Int = 0x08        // Vectorización de las interrupciones de alta prioridad
void High_Int(void) {
	_asm goto High_Int_Handler _endasm	
}
//#pragma code

#pragma interrupt High_Int_Handler
void High_Int_Handler (void) {      // Interrupciones de alta prioridad
    if (INTCONbits.TMR0IF) {
        INTCONbits.TMR0IF = 0;      // Se pone a '0' el flag de interrupción del temp. 0
        TMR0H = 15536/256;          // Se recarga el valor del temp. 0 para un temporización de 50ms
        TMR0L = 15536%256;          // Valor rec. = 65536 - (50*e-03*4*e+06)/4= 15536

        // Bloque I2C
        pressure = readPressure();
        ltoa(pressure,p);
        altitude = calcAltitude(101325,pressure);
        lWhole = (long)(altitude); ltoa(lWhole,lalt);
        ulPart = (long)(altitude*MULTIPLIER) - lWhole*MULTIPLIER; ultoa(ulPart,ralt);
        putsUSART(p); TXREG = ','; putsUSART(lalt); TXREG = '.'; putsUSART(ralt);
        TXREG = 0x0D;
        TXREG = 0x0A;
        
        // LED
        if (led_count < 5) {
            led_count += 1;
        } else {
            led_count = 0;
            LATCbits.LATC4 = !LATCbits.LATC4;
        }

        /*
        EEADR=num_dato;             // Se selecciona la posición de memoria en la EEPROM
        EEDATA=dato;                // Se prepara el dato a escribir en la EEPROM
        EECON1bits.WREN=1;          // Se habilita la escritura en la EEPROM
        INTCONbits.GIE = 0;         // Se deshabilitan las interrupciones (para evitar interrupciones durante la sec. de control)
        EECON2=0x55;                // Se escribe la secuencia 
        EECON2=0xAA;                //  de control para escribir en la EEPROM
        EECON1bits.WR=1;            // Se inicia el proceso de escritura
        INTCONbits.GIE = 1;         // Se vuelven a habilitar las interrupciones
        while (EECON1bits.WR==1);   // Se espera a que finalice el proceso de escritura en la EEPROM
        EECON1bits.WREN=0;          // Se deshabilita la escritura en la EEPROM

        num_dato=num_dato+1;        // Se incrementa el nº de datos
        if (num_dato==0) {          // Si se han tomado 256 muestra 
            T0CONbits.TMR0ON=0;     // se paraliza el proceso de muestreo
        }
        */
    }
}

void write8(unsigned char a, unsigned char d) {
    StartI2C();
    WriteI2C(BMP085_I2CADDR);
    WriteI2C(a);
    WriteI2C(d);
    StopI2C();
}

unsigned char read8(unsigned char a) {
    unsigned char ret;

    StartI2C();
    WriteI2C(BMP085_I2CADDR); // start transmission to device
    WriteI2C(a);
    RestartI2C();
    WriteI2C(BMP085_I2CADDR|0x01);
    ret = ReadI2C();
    NotAckI2C();
    StopI2C();

    return ret;
}

unsigned int read16(unsigned char a) {
    unsigned char string[2];
    unsigned int ret;

    StartI2C();
    WriteI2C(BMP085_I2CADDR);
    WriteI2C(a);
    RestartI2C();
    WriteI2C(BMP085_I2CADDR|0x01);
    getsI2C(string,2);
    NotAckI2C();
    StopI2C();
    ret = ((unsigned int)string[0]<<8) + string[1];

    return ret;
}

unsigned int readRawTemperature(void) {
    write8(BMP085_CONTROL, BMP085_READTEMPCMD);
    Delay1KTCYx(5);
    return read16(BMP085_TEMPDATA);
}

unsigned long readRawPressure(void) {
    unsigned long raw;
    
    write8(BMP085_CONTROL, BMP085_READPRESSURECMD + (oversampling << 6));
    if (oversampling == BMP085_ULTRALOWPOWER) 
        Delay1KTCYx(5);
    else if (oversampling == BMP085_STANDARD) 
        Delay1KTCYx(8);
    else if (oversampling == BMP085_HIGHRES) 
        Delay1KTCYx(14);
    else 
        Delay1KTCYx(26);
        
    raw = read16(BMP085_PRESSUREDATA);

    raw <<= 8;
    raw |= read8(BMP085_PRESSUREDATA + 2);
    raw >>= (8 - oversampling);
    
    return raw;
}

long readPressure(void) {
    long UT, UP, B3, B5, B6, X1, X2, X3, p;
    unsigned long B4, B7;

    UT = readRawTemperature();
    UP = readRawPressure();

    // do temperature calculations
    X1 = (UT-(long)(ac6)) * ((long)(ac5)) / 32768;
    X2 = ((long)mc*2048)/(X1 + (long)md);
    B5 = X1 + X2;
    
    // do pressure calculations
    B6 = B5 - 4000;
    X1 = ((long)b2 * ((B6 * B6) / 4096)) / 2048;
    X2 = ((long)ac2 * B6) / 2048;
    X3 = X1 + X2;
    B3 = ((((long)ac1*4 + X3) << oversampling) + 2) / 4;
    X1 = ((long)ac3 * B6) / 8192;
    X2 = ((long)b1 * ((B6 * B6) / 4096)) / 65536;
    X3 = ((X1 + X2) + 2) / 4;
    B4 = ((unsigned long)ac4 * (unsigned long)(X3 + 32768)) / 32768;
    B7 = ((unsigned long)UP - B3) * (unsigned long)(50000 >> oversampling);
    if (B7 < 0x80000000) {
        p = (B7 * 2) / B4;
    } else {
        p = (B7 / B4) * 2;
    }
    X1 = (p / 256) * (p / 256);
    X1 = (X1 * 3038) / 65536;
    X2 = (-7357 * p) / 65536;
    
    p = p + ((X1 + X2 + (long)3791) / 16);
    
    return p;
}

float readAltitude(float sealevelPressure) {
    float altitude;
    
    float pressure = readPressure();
    
    altitude = 44330 * (1.0 - pow(pressure/sealevelPressure,0.1903));
    
    return altitude;
}

float calcAltitude(float sealevelPressure, long pressure) {
    float altitude;
    
    altitude = 44330 * (1.0 - pow(pressure/sealevelPressure,0.1903));
    
    return altitude;
}

void main (void) {
    /* Inicializaciones */
    TRISCbits.TRISC0=0;             // RC0 como salida
    TRISCbits.TRISC4=0;             // RC4 como salida

    RCONbits.IPEN = 0;              // Se desactivan las prioridades en las interrupciones
    INTCONbits.GIE = 1;             // Se habilitan las interrupciones a nivel global

    /* Configuración Temporizador 0 */
    T0CON = 0b10001001;             // Configuración temp. 0 (en marcha, 16 bits, mode temp., sin prescalar)
    INTCONbits.TMR0IE = 1;          // Se habilita la interrupción del temp. 0
    TMR0H = 15536/256;              // Se recarga el valor del temp. 0 para un temporización de 100ms
    TMR0L = 15536%256;              //  Valor rec. = 65536 - (50*e-03*4*e+06)/4= 15536	

    /* Configuración canal EUSART */
    TRISBbits.TRISB7 = 0;           // Se configura RB7/TX como salida
    TRISBbits.TRISB5 = 1;           // Configure RB5/RX como entrada
    ANSELHbits.ANS11 = 0;           // Puerto en modo digital
    RCSTA = 0x90;                   // Cofiguración canal EUSART: RB7 y RB5 líneas TX y RX / Recepción habilitada
    TXSTA = 0x24;                   // Transmisión habilitada / BRGH='1'
    BAUDCON = 0x00;                 // BRG16 = '0'
    SPBRG= 25;                      // Velocidad de comunicación: 9600 => SPBRG = 4*e06/(16*9600)-1 = 25,042

    /* Configuración EEPROM */
    EECON1=0x00;                    // Se configura el acceso a la EEPROM (EEPGD='0' y CFGS='0')

    /* Configuración I2C */
    OpenI2C(MASTER, SLEW_OFF);
    SSPADD = 9;

    /* Configuración BMP085 */
    oversampling = mode;

    /* Read calibration data */
    ac1 = read16(BMP085_CAL_AC1);
    ac2 = read16(BMP085_CAL_AC2);
    ac3 = read16(BMP085_CAL_AC3);
    ac4 = read16(BMP085_CAL_AC4);
    ac5 = read16(BMP085_CAL_AC5);
    ac6 = read16(BMP085_CAL_AC6);

    b1 = read16(BMP085_CAL_B1);
    b2 = read16(BMP085_CAL_B2);

    mb = read16(BMP085_CAL_MB);
    mc = read16(BMP085_CAL_MC);
    md = read16(BMP085_CAL_MD);

    led_count = 0;
    
    LATCbits.LATC4 = 0;

    while (1);
}