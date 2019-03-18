/*
 * GccApplication3.c
 *
 * Created: 18/03/2019 17:18:29
 * Author : Kristijan
 */ 

#define F_CPU 8000000UL

#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lcd.h"
#include "util/delay.h"
#define DHT11_PIN 7
uint8_t c=0,I_RH,D_RH,I_Temp,D_Temp,CheckSum;

void Request()				/* Microcontroller send start pulse/request */
{
	DDRA |= (1<<DHT11_PIN);
	PORTA &= ~(1<<DHT11_PIN);	/* set to low pin */
	_delay_ms(20);			/* wait for 20ms */
	PORTA |= (1<<DHT11_PIN);	/* set to high pin */
}

void Response()				/* receive response from DHT11 */
{
	DDRA &= ~(1<<DHT11_PIN);
	while(PINA & (1<<DHT11_PIN));
	while((PINA & (1<<DHT11_PIN))==0);
	while(PINA & (1<<DHT11_PIN));
}

uint8_t Receive_data()			/* receive data */
{
	for (int q=0; q<8; q++)
	{
		while((PINA & (1<<DHT11_PIN)) == 0);  /* check received bit 0 or 1 */
		_delay_us(30);
		if(PINA & (1<<DHT11_PIN))/* if high pulse is greater than 30ms */
		c = (c<<1)|(0x01);	/* then its logic HIGH */
		else			/* otherwise its logic LOW */
		c = (c<<1);
		while(PINA & (1<<DHT11_PIN));
	}
	return c;
}

//SOIL

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


	char data[5];


int main(void)
{
	
	
	DDRD = _BV(4);

	TCCR1A = _BV(COM1B1) | _BV(WGM10);
	TCCR1B = _BV(WGM12) | _BV(CS11);
	OCR1B = 80;
	

	lcd_init(LCD_DISP_ON);			/* Initialize LCD */
	lcd_clrscr();			/* Clear LCD */
	lcd_gotoxy(0,0);		/* Enter column and row position */
	lcd_puts("H =");
	lcd_gotoxy(0,1);
	lcd_puts("T = ");


	ADC_Init();		/* initialize the ADC */
	char array[10];
	int adc_value;
	float moisture;

	//_delay_ms(100);

	while(1)
	{	
		
		Request();		/* send start pulse */
		Response();		/* receive response */
		I_RH=Receive_data();	/* store first eight bit in I_RH */
		D_RH=Receive_data();	/* store next eight bit in D_RH */
		I_Temp=Receive_data();	/* store next eight bit in I_Temp */
		D_Temp=Receive_data();	/* store next eight bit in D_Temp */
		CheckSum=Receive_data();/* store next eight bit in CheckSum */
		
		if ((I_RH + D_RH + I_Temp + D_Temp) != CheckSum)
		{
			lcd_gotoxy(0,0);
			lcd_puts("Error");
		}
		
		else
		{
			itoa(I_RH,data,10);
			lcd_gotoxy(4,0);
			lcd_puts(data);
			lcd_puts("%");

			itoa(I_Temp,data,10);
			lcd_gotoxy(4,1);
			lcd_puts(data);
			lcd_putc(0xDF);
			lcd_puts("C ");
			
			//itoa(checksum,data,10);
			//lcd_puts(data);
			//lcd_puts("b");


			// SOIL

			//lcd_gotoxy(9,0);
			//lcd_puts("H = 00%");


		}

		adc_value = ADC_Read();	/* Copy the ADC value */
		moisture = 100-(adc_value*100.00)/1023.00; /* Calculate moisture in % */
		lcd_gotoxy(9,0);
		lcd_puts("M: ");
		dtostrf(moisture,3,2,array);
		strcat(array,"%   ");	/* Concatenate unit of % */
		lcd_gotoxy(9,1);	/* set column and row */
		lcd_puts(array);	/* Print moisture on second row */
		memset(array,0,10);
		_delay_ms(500);
		
		//_delay_ms(500);
	}

}