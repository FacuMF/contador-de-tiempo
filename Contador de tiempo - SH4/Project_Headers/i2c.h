/*
 * i2c.h
 *
 *  Created on: Aug 7, 2020
 *      Author: Usuario
 */

#ifndef I2C_H_
#define I2C_H_

/*  
    escritura - lectura de memoria IIC 24Cxxxx con S08
  
  - en este ejemplo se utilizó un QG8 y una memoria 24C65
  - en otros modelos tener en cuenta el tamaño del cache de escritura
    (Ej. 24C02 máximo 8 bytes) 
  - se deja ID y dirección hardcoded en 1010 000 por simplicidad
    modificarlo para otros dispositivios IIC 
 
    MCU (SH8 - QG8)            24C65
  PTB6 (6) <-- SDA --> (5) SDA      (1), (2), (3), (4) VSS
  PTB4 (8) --- SCL --> (6) SCL      (8) Vcc
  
  --> Recordar R pull-up (Ej. 4K7) en SDA y SCL
  
*/

#define SDA PTAD_PTAD2
#define SCL PTAD_PTAD3
#define salida 1
#define entrada 0

#define DDR_SDA PTADD_PTADD2 // bit config. E/S pin SDA
#define DDR_SCL PTADD_PTADD3 // bit config. E/S pin SDA

/*   funciones I2C   */
void I2Cdelay(void);
void i2c_tx(unsigned char byte);
void i2c_addr(unsigned char addr, unsigned char rw);
unsigned char i2c_rx(unsigned char ultimo);
void i2c_stop(void);
void i2c_start(void);


/*   funciones para lectura / escritura de conjunto de bytes (sequential read / write)  */
void leer_memo( unsigned char* buffer, unsigned int direccion, unsigned char cantidad); 
void escribir_memo( const unsigned char* buffer, unsigned int direccion, unsigned char cantidad);

/*   funciones para lectura / escritura de un bytes (random read / write) */
unsigned char leer_byte( unsigned int addr );
void escribir_byte( unsigned int direccion, unsigned char dato );

#endif /* I2C_H_ */
