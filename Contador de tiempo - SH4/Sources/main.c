#include <hidef.h> /* for EnableInterrupts macro */
#include "derivative.h" /* include peripheral declarations */
#include "main.h"
#include <stdlib.h>

enum {
	cambioDeModo, modoNormal, modoServicio
};

char confirmacionPendiente = 0;

char usoActual = BAJO;

char contadorSegundos = 0;
char contadorMinutos = 0;
int cantidadHoras = 0;

unsigned char digito[10] = { 0b11111100, 0b01100000, 0b11011010, 0b11110010,
		0b01100110, 0b10110110, 0b10111110, 0b11100000, 0b11111110, 0b11110110 };
char estado = modoNormal, estadoAnterior = modoServicio;

char timeOutServicio = 0;

char minutoCumplido = 0;

char pOnOff = 0, pCV = 0, auxOnOff = 0, auxCV = 0;

char buf[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
char bf[10];
void main(void) {
	char estadoOnOff = 0;

	char estadoCV = 0;
	SOPT1 &= 0x3F;
	EnableInterrupts
	;

	initClock();
	inicializacionPuertos();
	inicializacionPinInterrupts();
	RTC_Init();

	encender();

	//escribir_memo(buf,0,10);
	//leer_memo(bf, 0, 10);

	for (;;) {
		if (minutoCumplido) {
			escribir_byte(9, contadorMinutos);
			escribir_byte(9, contadorMinutos);
			escribirHorasEnMemoria(cantidadHoras, PRIMARIA);

			mostrarHorasEnDisplay(cantidadHoras);

			mostrarMinutos(contadorMinutos);
			minutoCumplido = 0;
		}

		if (ON_OFF == 0 && CV == 0)
			estado = cambioDeModo;

		switch (estado) {
		case cambioDeModo:
			estado = iniciarCambioDeModo(estadoAnterior);
			break;
		case modoNormal:
			estadoAnterior = estado;
			estadoOnOff = estadoPOnOff(&pOnOff, &auxOnOff);
			estadoCV = estadoPCV(&pCV, &auxCV);

			if (estadoOnOff)
				leer_memo(buf, 0, 10);

			usoActual = determinarUso(cantidadHoras);
			mostrarUso(usoActual);
			mostrarNumero(GUION);

			break;
		case modoServicio:
			estadoAnterior = estado;
			estadoOnOff = estadoPOnOff(&pOnOff, &auxOnOff);
			estadoCV = estadoPCV(&pCV, &auxCV);

			if (timeOutServicio >= TIME_OUT) {
				indicarCambioDeModo();
				estado = modoNormal;
				timeOutServicio = 0;
			}

			if (confirmacionPendiente) {
				mostrarNumero(CONFIRMACION);
				if (estadoOnOff == 1) {
					reiniciarHoras();
					confirmacionPendiente = 0;
					notificarConBuzzer();
				}
				if (estadoCV) {
					confirmacionPendiente = 0;
					notificarConBuzzer();
				}
			} else {
				mostrarNumero(GUION); //Muestro un giuon que indica que me encuentro en modo servicio
				if (estadoOnOff)
					confirmacionPendiente = 1;
				if (estadoCV){
					mostrarHorasEnDisplay(cantidadHoras);
					mostrarMinutos(contadorMinutos);
				}
			}
			break;
		default:
			estado = modoNormal;
		}
	}
}

void initClock() {
	/* ICSC1: CLKS=0,RDIV=0,IREFS=1,IRCLKEN=1,IREFSTEN=0 */
	ICSC1 = 0x06U; /* Initialization of the ICS control register 1 */
	/* ICSC2: BDIV=1,RANGE=0,HGO=0,LP=0,EREFS=1,ERCLKEN=1,EREFSTEN=0 */ICSC2 =
			0x46U; /* Initialization of the ICS control register 2 */
	while (ICSSC_IREFST == 0U) { /* Wait until the source of reference clock is internal clock */
	}
}

void RTC_Init(void) {
	/* RTCMOD: RTCMOD=0x1F */
	RTCMOD = 0x1FU; /* Set modulo register */
	/* RTCSC: RTIF=1,RTCLKS=1,RTIE=1,RTCPS=1 */
	RTCSC = 0xB1U; /* Configure RTC */
}

void inicializacionPuertos() {
	_USO_BAJO = SALIDA;
	_USO_MEDIO = SALIDA;
	_USO_ALTO = SALIDA;
	_BUZZER = SALIDA;
	_TR = SALIDA;
	_DATA = SALIDA;
	_CLK = SALIDA;

	USO_BAJO = APAGADO;
	USO_MEDIO = APAGADO;
	USO_ALTO = APAGADO;
	BUZZER = APAGADO;
	TR = ENCENDIDO;
	DATA = APAGADO;
	CLK = APAGADO;

	_ON_OFF = ENTRADA;
	_CV = ENTRADA;
}

void inicializacionPinInterrupts() {
	/*1. Mask interrupts by clearing PTxIE in PTxSC.
	 2. Select the pin polarity by setting the appropriate PTxESn bits in PTxES.
	 3. If using internal pull-up/pull-down device, configure the associated pull enable bits in PTxPE.
	 4. Enable the interrupt pins by setting the appropriate PTxPSn bits in PTxPS.
	 5. Write to PTxACK in PTxSC to clear any false interrupts.
	 6. Set PTxIE in PTxSC to enable interrupts.*/

	/*PTASC_PTAIE = 0; //limpio flag de interrupciones
	PTAES_PTAES1 = 1; // Detecta flancos ascendentes

	PTAPS_PTAPS1 = 1; //Activo interrupciones para DC0
	PTASC = 0b00001110; //PTAACK = 1, PTAIE = 1, PTAMOD = 0*/

	//Rpull up memoria
	PTAPE_PTAPE2 = 1;
	PTAPE_PTAPE3 = 1;
	PTAES_PTAES2 = 0;
	PTAES_PTAES3 = 0;

}

char iniciarCambioDeModo(char estadoAnterior) {
	int i;
	char reinicioCompleto = 0;
	TPM2SC = 0b0001111;
	TPM2MOD = 62499; //1s
	for (i = 0; i < TIEMPO_REINICIO; i++) {
		while (ON_OFF == 0 && CV == 0 && TPM2SC_TOF == 0)
			;
		TPM2SC_TOF = 0;
		if (ON_OFF == 0 && CV == 0)
			reinicioCompleto++;
	}
	TPM2SC = 0;
	if (reinicioCompleto == TIEMPO_REINICIO) {
		reinicioCompleto = 0;
		indicarCambioDeModo();
		if (estadoAnterior == modoNormal) {
			auxOnOff = 0;
			auxCV = 0;
			pOnOff = 0;
			pCV = 0;
			return modoServicio;
		}
		if (estadoAnterior == modoServicio) {
			auxOnOff = 0;
			auxCV = 0;
			pOnOff = 0;
			pCV = 0;
			return modoNormal;
		}
	} else {
		reinicioCompleto = 0;
		auxOnOff = 0;
		auxCV = 0;
		pOnOff = 0;
		pCV = 0;
		return estadoAnterior;
	}
}

void indicarCambioDeModo() {
	int i;
	unsigned char a = 0b11111111;
	mostrarNumero(a);
	for (i = 0; i < 6; i++) {
		a = ~a;
		mostrarNumero(a);
		delay(5);
	}
}

void reiniciarHoras() {
	cantidadHoras = 0;
	contadorMinutos = 0;
	escribirHorasEnMemoria(cantidadHoras, PRIMARIA);
	escribir_byte(9,contadorMinutos);
	mostrarHorasEnDisplay(cantidadHoras);
	mostrarMinutos(contadorMinutos);
}

void notificarConBuzzer(void) {
	int i;
	for (i = 0; i < 10000; i++)
		BUZZER = ENCENDIDO;
	BUZZER = APAGADO;
}

void mostrarUso(char estado) {
	switch (estado) {
	case BAJO:
		USO_BAJO = ENCENDIDO;
		USO_MEDIO = APAGADO;
		USO_ALTO = APAGADO;
		break;
	case MEDIO:
		USO_BAJO = APAGADO;
		USO_MEDIO = ENCENDIDO;
		USO_ALTO = APAGADO;
		break;
	case ALTO:
		USO_BAJO = APAGADO;
		USO_MEDIO = APAGADO;
		USO_ALTO = ENCENDIDO;
		break;
	case OFF:
		USO_BAJO = APAGADO;
		USO_MEDIO = APAGADO;
		USO_ALTO = APAGADO;
		break;
	default:
		USO_BAJO = APAGADO;
		USO_MEDIO = APAGADO;
		USO_ALTO = APAGADO;
	}
}

char estadoPOnOff(char* pOnOff, char* aux) {
	switch (*pOnOff) {
	case 0:
		*aux = 0;
		if (ON_OFF == PULSADO)
			*pOnOff = 1;
		break;
	case 1:
		*aux = 1;
		if (ON_OFF == ABIERTO)
			*pOnOff = 0;
		if (ON_OFF == PULSADO)
			*pOnOff = 2;
		break;
	case 2:
		*aux = 0;
		if (ON_OFF == ABIERTO)
			*pOnOff = 0;
		break;
	default:
		*aux = 0;
		*pOnOff = 0;
	}
	return *aux;
}

char estadoPCV(char* pCV, char* aux) {
	switch (*pCV) {
	case 0:
		*aux = 0;
		if (CV == PULSADO)
			*pCV = 1;
		break;
	case 1:
		*aux = 1;
		if (CV == ABIERTO)
			*pCV = 0;
		if (CV == PULSADO)
			*pCV = 2;
		break;
	case 2:
		*aux = 0;
		if (CV == ABIERTO)
			*pCV = 0;
		break;
	default:
		*aux = 0;
		*pCV = 0;
	}
	return *aux;
}

void indicarVelocidadElegida(unsigned char velElegida) {
	mostrarNumero(digito[velElegida]);
}

void encender() {
	cantidadHoras = leerHorasDeMemoria();
	cantidadHoras = leerHorasDeMemoria();
	contadorMinutos = leer_byte(9);
	contadorMinutos = leer_byte(9);
	notificarConBuzzer();
}

int leerHorasDeMemoria() {
	int horasLeidas;
	unsigned char horas[4];

	leer_memo(horas, 0, 4);
	horasLeidas = horas[0] * 1000 + horas[1] * 100 + horas[2] * 10 + horas[3];

	return horasLeidas;

}

__interrupt 25 void rtcInt(void) {
	RTCSC_RTIF = 1;

	if (estado == modoServicio)
		timeOutServicio++;

	contadorSegundos++;

	if (contadorSegundos >= 60) {
		contadorSegundos = 0;
		contadorMinutos++;
		minutoCumplido = 1;
	}
	if (contadorMinutos >= 60) {
		contadorMinutos = 0;
		contadorSegundos = 0;
		cantidadHoras++;
	}
}

void escribirHorasEnMemoria(int horas, int posicion) {
	unsigned char horasStr[4];

	horasStr[0] = horas / 1000;
	horasStr[1] = horas / 100 - horasStr[0] * 10;
	horasStr[2] = horas / 10 - horasStr[0] * 100 - horasStr[1] * 10;
	horasStr[3] = horas - horasStr[0] * 1000 - horasStr[1] * 100
			- horasStr[2] * 10;

	escribir_memo(horasStr, posicion, 4);
}

void mostrarNumero(unsigned char digitoMostrado) {
	int i;
	_DATA = SALIDA;
	_CLK = SALIDA;
	DATA = APAGADO;
	CLK = APAGADO;

	for (i = 0; i < 8; i++) {
		DATA = digitoMostrado & 1;
		CLK = 1;
		CLK = 0;
		digitoMostrado = digitoMostrado >> 1;
	}
	for (i = 0; i < 20000; i++)
		;
	for (i = 0; i < 20000; i++)
		;
	for (i = 0; i < 20000; i++)
		;
}

void mostrarHorasEnDisplay(int horas) {
	int horasAMostrar[4];
	char i;
	int j;
	for (i = 0; i < 4; i++) {
		horasAMostrar[3 - i] = horas % 10;
		horas /= 10;
	}

	for (i = 0; i < 4; i++) {
		//notificarConBuzzer();
		mostrarNumero(digito[horasAMostrar[i]]);
		delay(10);
		mostrarNumero(0b00000000);
		for (j = 0; j < 20000; j++)
			;
	}
}

void mostrarMinutos(char minutos) {
	char decena, unidad;
	decena = minutos / 10;
	unidad = minutos - 10 * decena;
	mostrarNumero(digito[decena]);
	delay(10);
	mostrarNumero(0b00000000);
	mostrarNumero(digito[unidad]);
	delay(10);
	mostrarNumero(0b00000000);
}
