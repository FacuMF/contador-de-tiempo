#include <hidef.h> /* for EnableInterrupts macro */
#include "derivative.h" /* include peripheral declarations */
#include "i2c.h"

/*   variable necesaria para I2C   */
unsigned char i2c_error; // bit error

/*   variables para este ejemplo   */
unsigned char i;
unsigned char buff[ 20 ];


/* escribe cantidad de bytes desde dirección - para 24Cxx */
void escribir_memo( const unsigned char* buffer, unsigned int direccion, unsigned char cantidad) {
	for(i=0;i<cantidad;i++)
		escribir_byte(i+direccion,buffer[i]);
	
}

/* lee cantidad bytes desde a direccion - para 24Cxx */
void leer_memo( unsigned char* buffer, unsigned int direccion, unsigned char cantidad){
	unsigned char p;	
	unsigned char byte_dire;
	
	/* enviar start */
	i2c_start();
	                    // para 24C...   A2 A1 A0 
	i2c_tx(0b10100000); //    1010        0  0  0   0 = escribir	
	
	/* enviar address hi */
	byte_dire = 0;	
	i2c_tx( byte_dire ); 
	
	/* enviar address lo */
	byte_dire = direccion & 0x00FF;	
	i2c_tx( byte_dire ); 

	I2Cdelay();
	
	/* leer datos */
	p = 0; 
	while(p < cantidad){
		i2c_start();

			            // para 24C...   A2 A1 A0 
	        i2c_tx(0b10100001); //    1010        0  0  0   1 = leer
		
		*(buffer + p) = i2c_rx(1);
		p++;
		
		i2c_stop();

		I2Cdelay();		
	}
}

/* escribe un byte a direccion - para 24Cxx */
void escribir_byte( unsigned int direccion, unsigned char dato ) {
	unsigned char byte_dire;
	
	/* enviar start */
	i2c_start();
	
	                    // para 24C...   A2 A1 A0 
	i2c_tx(0b10100000); //    1010        0  0  0   0 = escribir	
	
	/* enviar address hi */
	byte_dire = 0;	
	i2c_tx( byte_dire );
	
	/* enviar address lo */
	byte_dire = direccion & 0x00FF;
	i2c_tx( byte_dire ); 

	I2Cdelay();
	
	/* enviar dato */
	i2c_tx( dato );
	I2Cdelay();		

	/* enviar stop */
	i2c_stop();		

	/* espera fin ciclo escritura (ACK polling) */	
	do {
		i2c_start();
				    // para 24C...   A2 A1 A0 
		i2c_tx(0b10100000); //    1010        0  0  0   0 = escribir	
	} while ( i2c_error == 1);		
}

/* devuelve byte en direccion - para 24Cxx */
unsigned char leer_byte(unsigned int direccion) {
	unsigned char aux;
	unsigned char byte_dire;

	/* enviar start */
	i2c_start();
	// para 24C...   A2 A1 A0 
	i2c_tx(0b10100000); //    1010        0  0  0   0 = escribir	

	/* enviar address hi */
	byte_dire = 0;
	i2c_tx(byte_dire);

	/* enviar address lo */
	byte_dire = direccion & 0x00FF;
	i2c_tx(byte_dire);

	I2Cdelay();

	/* enviar start */
	i2c_start();
	// para 24C...   A2 A1 A0 
	i2c_tx(0b10100001); //    1010        0  0  0   1 = leer

	/* obtiene byte */
	aux = i2c_rx(1);

	/* enviar stop */
	i2c_stop();

	I2Cdelay();

	return aux;
}

/* --- funciones generales para IIC --- */
void I2Cdelay(void) 
{
	unsigned char i;

	for (i = 0; i < 10; i++); 
}

void i2c_tx(unsigned char byte)
{
	unsigned char i;

	DDR_SDA = salida; // SDA salida

	for (i = 0; i < 8; i++)
	{
		if (byte & 0x80)
			SDA = 1; //envia cada bit, comenzando por el MSB
		else
			SDA = 0;

		SCL = 1;

		I2Cdelay();

		SCL = 0;

		I2Cdelay();

		byte = byte << 1;
	}

	DDR_SDA = entrada; // SDA entrada ; espera ACK (config. pin como lectura)

	SCL = 1;

	I2Cdelay();

	if (SDA)
		i2c_error = 0x01; // error si el ACK no es correcto
	else 
		i2c_error = 0;

	SCL = 0;

	I2Cdelay();
}

void i2c_start(){
	DDR_SDA = salida; // SDA salida

	SDA = 1; // garantizamos el estado inicial de SDA y SCL (reposo)

	SCL = 1;

	I2Cdelay();

	SDA = 0; // condicion START

	I2Cdelay();

	SCL = 0;

	I2Cdelay();	
}

void i2c_addr(unsigned char addr, unsigned char rw)
{

	// addr, dirección (7 bits)
	// rw=1, lectura
	// rw=0, escritura

	i2c_tx(addr << 1 + rw); // envia byte de direccion
}

unsigned char i2c_rx(unsigned char ultimo)
// ultimo = NACK = 1 para el ultimo byte
// ultimo = ACK = 0 para cualquier byte que no sea el último.
{
	unsigned char i, byte;

	byte = 0;

	DDR_SDA = entrada; //SDA entrada

	for (i=0;i<8;i++)// lee un bit comenzando por el MSB
	{
		//SCL=1;
		
		do {
			DDR_SCL = salida;
			SCL = 1;
			DDR_SCL = entrada;
		}  while(SCL==0); // SCL ck stretching?
		
		DDR_SCL = salida;
		
		I2Cdelay();

		byte<<=1;

		if (SDA) byte++;

		SCL = 0;

		I2Cdelay();
	}

	DDR_SDA = salida; // SDA salida

	SDA = ultimo;// ACK es funcion de ultimo

	SCL=1;// ACK=0, si no es último byte leído

	I2Cdelay();// ACK=1, después del último byte leído (NACK)

	SCL = 0;

	I2Cdelay();

	return(byte);
}

void i2c_stop(void)
{
	DDR_SDA = salida; // SDA salida

	SDA = 0;

	I2Cdelay();

	SCL = 1;

	I2Cdelay();

	SDA = 1;

	I2Cdelay();
}
