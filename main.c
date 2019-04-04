/*
 * URS_projekt - Soil moisture measure
 *
 * Created: 18/03/2019 17:18:29
 * Author : Kristijan, Bruno, Matija
 */ 

#define F_CPU 8000000UL				/* Define CPU Frequency e.g. here its Ext. 12MHz */
#include <avr/io.h>					/* Include AVR std. library file */
#include <util/delay.h>				/* Include Delay header file */
#include <stdbool.h>				/* Include standard boolean library */
#include <string.h>					/* Include string library */
#include <stdio.h>					/* Include standard IO library */
#include <stdlib.h>					/* Include standard library */
#include <avr/interrupt.h>			/* Include avr interrupt header file */
#include "USART_RS232_H_file.h"		/* Include USART header file */

#define SREG    _SFR_IO8(0x3F)		/* Mozda maknuti */

#define DEFAULT_BUFFER_SIZE		160

/* Connection Mode */
#define SINGLE					0
#define MULTIPLE				1

/* Application Mode */
#define NORMAL					0
#define TRANSPERANT				1

/* Application Mode */
#define STATION							1
#define ACCESSPOINT						2
#define BOTH_STATION_AND_ACCESPOINT		3

/* Define Required fields shown below */
#define DOMAIN				"api.thingspeak.com"
#define PORT				"80"
#define API_WRITE_KEY		"H4U6085PT48J0V3S"
#define CHANNEL_ID			"742903"

/* Define Wireless connection */
#define SSID				"ISKONOVAC-5527ff"
#define PASSWORD			"ISKON2802508179"

/* Define DHT11 Pin and TIMEOUT */
#define DHT11_PIN 4
#define TIMEOUT 200

int8_t Response_Status;
volatile int16_t Counter = 0, pointer = 0;
char RESPONSE_BUFFER[DEFAULT_BUFFER_SIZE];

uint8_t c = 0;

void Request()				/* Microcontroller send start pulse/request */
{
	DDRB |= (1<<DHT11_PIN);
	PORTB &= ~(1<<DHT11_PIN);	/* set to low pin */
	_delay_ms(20);			/* wait for 20ms */
	PORTB |= (1<<DHT11_PIN);	/* set to high pin */
}

void Response()	{			/* receive response from DHT11 */
	DDRB &= ~ _BV(DHT11_PIN);
	
	uint8_t timeo = TIMEOUT;
	while(PINB & _BV(DHT11_PIN) && timeo--);
	
	timeo = TIMEOUT;
	while(!(PINB & _BV(DHT11_PIN)) && timeo--);

	timeo = TIMEOUT;
	while(PINB & _BV(DHT11_PIN) && timeo--);
}

uint8_t Receive_data() { /* receive data */
	uint8_t timeo = TIMEOUT;
	
	for (int q=0; q<8; q++)	{
		while(((PINB & (1<<DHT11_PIN)) == 0) && timeo--);  /* check received bit 0 or 1 */
		_delay_us(30);
		if(PINB & (1<<DHT11_PIN))/* if high pulse is greater than 30ms */
		c = (c<<1)|(0x01);	/* then its logic HIGH */
		else			/* otherwise its logic LOW */
		c = (c<<1);
		timeo = TIMEOUT;
		while((PINB & (1<<DHT11_PIN)) && timeo--);
	}
	return c;
}

void ADC_Init()
{
	DDRA=0x0;		/*  Make ADC port as input  */
	ADCSRA = 0x87;		/*  Enable ADC, fr/128  */
}

int ADC_Read()
{
	ADMUX = 0x40;		/* Vref: Avcc, ADC channel: 0  */
	ADCSRA |= (1<<ADSC);	/* start conversion  */
	while ((ADCSRA &(1<<ADIF))==0);	/* monitor end of conversion interrupt flag */
	ADCSRA |=(1<<ADIF);	/* set the ADIF bit of ADCSRA register */
	return(ADC);		/* return the ADCW */
}

ISR (USART_RXC_vect)
{
	uint8_t oldsrg = SREG;
	cli();
	RESPONSE_BUFFER[Counter] = UDR;
	Counter++;
	if(Counter == DEFAULT_BUFFER_SIZE){
		Counter = 0; pointer = 0;
	}
	SREG = oldsrg;
}

int main(void)
{
	
	ADC_Init();

	USART_Init(115200);						/* Initiate USART with 115200 baud rate */
	sei();									/* Start global interrupt */
	
	USART_SendString("AT+RST");
	USART_SendString("\r\n");
	_delay_ms(10000);						/* Cekaj da se wifi uspostavi, za sad */
	
	char _buffer[DEFAULT_BUFFER_SIZE];
	char _comm[DEFAULT_BUFFER_SIZE];
	memset(_buffer, 0, DEFAULT_BUFFER_SIZE);
	memset(_comm, 0, DEFAULT_BUFFER_SIZE);
	
	uint8_t I_RH, D_RH, I_Temp, D_Temp, CheckSum; /* Varijable za DHT11 */
	
	while(1)
	{
		
		Request();				// send start pulse
		Response();				// receive response
		I_RH=Receive_data();	// store first eight bit in I_RH
		D_RH=Receive_data();	// store next eight bit in D_RH
		I_Temp=Receive_data();	// store next eight bit in I_Temp
		D_Temp=Receive_data();	// store next eight bit in D_Temp
		CheckSum=Receive_data();// store next eight bit in CheckSum
		
		int adc_value = ADC_Read();	/* Copy the ADC value */
		float moisture = 100-(adc_value*100.00)/1023.00; /* Calculate moisture in % */
		
		
		// Send
		memset(_buffer, 0, DEFAULT_BUFFER_SIZE);
		sprintf(_buffer, "GET /update?api_key=H4U6085PT48J0V3S&field1=%d&field2=%d&field3=%d.%d", I_Temp, I_RH, (int)moisture, (int)((moisture-(int)(moisture))*100));
		//sprintf(_buffer, "GET /update?api_key=H4U6085PT48J0V3S&field1=%d&field2=%d&field3=%d.%d", 1, 2, (int)3.33, (int)((4-(int)(4))*100));
		//sprintf(_buffer, "GET /update?api_key=H4U6085PT48J0V3S&field1=%d&field2=%d&field3=%.2f", 1, 2, 3.22);
		
		USART_SendString("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80");
		USART_SendString("\r\n");
		_delay_ms(1000);
		
		memset(_comm, 0, DEFAULT_BUFFER_SIZE);
		sprintf(_comm, "AT+CIPSEND=%d", (strlen(_buffer)+2));
		USART_SendString(_comm);
		USART_SendString("\r\n");
		_delay_ms(1000);


		USART_SendString(_buffer);
		USART_SendString("\r\n");
		_delay_ms(20000);
	}
}
