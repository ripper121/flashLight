/*
* FlashLight.cpp
*
* Created: 19.04.2017 15:57:32
* Author : sts
*/
// 9.6 MHz, built in resonator
#define LED PB1
#define F_CPU 9600000UL  // 1 MHz
#include <util/delay.h>
#include <avr/io.h>
#include <avr/eeprom.h>
//EEPROM Variables
uint8_t EEMEM MODE_P;
uint8_t EEMEM LEVEL_P;
uint8_t EEMEM BATTERY_P;
//if you want all set it to 6
#define MODE_COUNT 3

void ADC_Init(void)
{
	// die Versorgungsspannung AVcc als Referenz wählen:
	ADMUX = (1<<REFS0);
	// oder interne Referenzspannung als Referenz für den ADC wählen:
	// ADMUX = (1<<REFS1) | (1<<REFS0);

	// Bit ADFR ("free running") in ADCSRA steht beim Einschalten
	// schon auf 0, also single conversion
	ADCSRA = (1<<ADPS1) | (1<<ADPS0);     // Frequenzvorteiler
	ADCSRA |= (1<<ADEN);                  // ADC aktivieren

	/* nach Aktivieren des ADC wird ein "Dummy-Readout" empfohlen, man liest
	also einen Wert und verwirft diesen, um den ADC "warmlaufen zu lassen" */

	ADCSRA |= (1<<ADSC);                  // eine ADC-Wandlung
	while (ADCSRA & (1<<ADSC) ) {         // auf Abschluss der Konvertierung warten
	}
	/* ADCW muss einmal gelesen werden, sonst wird Ergebnis der nächsten
	Wandlung nicht übernommen. */
	(void) ADCW;
}

uint16_t ADC_Read( uint8_t channel )
{
	// Kanal waehlen, ohne andere Bits zu beeinflußen
	ADMUX = (ADMUX & ~(0x1F)) | (channel & 0x1F);
	ADCSRA |= (1<<ADSC);            // eine Wandlung "single conversion"
	while (ADCSRA & (1<<ADSC) ) {   // auf Abschluss der Konvertierung warten
	}
	return ADCW;                    // ADC auslesen und zurückgeben
}

uint16_t ADC_Read_Avg( uint8_t channel, uint8_t nsamples )
{
	uint32_t sum = 0;

	for (uint8_t i = 0; i < nsamples; ++i ) {
		sum += ADC_Read( channel );
	}

	return (uint16_t)( sum / nsamples );
}

void pwm_setup (void)
{
	// Set Timer 0 prescaler to clock/8.
	// At 9.6 MHz this is 1.2 MHz.
	// See ATtiny13 datasheet, Table 11.9.
	TCCR0B |= (1 << CS01);
	
	// Set to 'Fast PWM' mode
	TCCR0A |= (1 << WGM01) | (1 << WGM00);
	
	// Clear OC0B output on compare match, upwards counting.
	TCCR0A |= (1 << COM0B1);
}

void pwm_write (unsigned char val)
{
	OCR0B = val;
}

int main (void)
{
	ADC_Init();
	pwm_setup();
	// LED is an output.
	DDRB |= (1 << LED);
	pwm_write(0);

	
	volatile uint8_t	mode = eeprom_read_byte(&MODE_P);
	volatile uint8_t	prev_mode = mode;
	volatile uint8_t	customlevel = eeprom_read_byte(&LEVEL_P);
	volatile uint8_t	level = 0,battery=0xFFFF;
	
	if(mode>MODE_COUNT){
		mode=0;
		eeprom_busy_wait(); //make sure eeprom is ready
		eeprom_write_byte(&MODE_P, mode); // save mode
		}else{
		//Switch to next mode
		mode++;
		eeprom_busy_wait(); //make sure eeprom is ready
		eeprom_write_byte(&MODE_P, mode); // save mode

		pwm_write(2);
		_delay_ms(200);
		
		//Stay in Mode
		mode = prev_mode;
		eeprom_busy_wait(); //make sure eeprom is ready
		eeprom_write_byte(&MODE_P, mode); // save mode
	}
	pwm_write(0);
	_delay_ms(100);
	prev_mode = mode;
	
	while (1) {
		switch(mode){
			case 0:
			//Super low mode (you can go lower but this works with my Cree® XLamp® XP-L LED)
			level=2;
			break;
			case 1:
			//Low mode
			level=20;
			break;
			case 2:
			//Medium mode
			level=127;
			break;
			case 3:
			//Full mode
			level=255;
			break;
			case 4:
			//Flicker mode
			while(1){
				pwm_write(255);
				_delay_ms(50);
				pwm_write(0);
				_delay_ms(50);
			}
			break;
			case 5:
			//Set level for custom brightness mode, if you have the right Level turn off the Flashlight and switch to Mode 6
			while(1){
				if(customlevel==2)
				_delay_ms(3000); //Wait on lowest mode
				if(customlevel==100)
				_delay_ms(3000); //Wait on highest mode
				pwm_write(customlevel);
				eeprom_busy_wait(); //make sure eeprom is ready
				eeprom_write_byte(&LEVEL_P, customlevel); // save mode
				customlevel++;
				if(level>100)
				customlevel=2;
				_delay_ms(200);
			}
			break;
			case 6:
			//Custom brightness mode
			level = customlevel;
			break;
			
			default:
			//in case the EEPROM is defekt this mode will be always possible
			level=127;
			break;
		}
		pwm_write(level); //Set PWM for Led
		if(battery<=0x81){
			if(level>2){
				pwm_write(2); //Set PWM for Led
			}else{
				pwm_write(0); //Set PWM for Led
			}
			_delay_ms(1000);
			pwm_write(level); //Set PWM for Led
			_delay_ms(10000);
		}else{
			_delay_ms(10000);
		}
		
		battery = ADC_Read_Avg(1,5); // Read Battery Voltage 0x81 was with my Driver ~3.00V
	}
}