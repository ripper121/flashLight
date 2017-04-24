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
#define MODE_COUNT 6


#define adcchn 1

#define adcinit() do{ ADMUX =0b01100000|adcchn; ADCSRA=0b11000100; }while(0) //ref1.1V, left-adjust, ADC1/PB2; enable, start, clk/16


uint8_t adcread(){
	ADCSRA|=64;
	while (ADCSRA&64);
	return ADCH;
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

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


int main (void)
{
	adcinit();
	pwm_setup();
	// LED is an output.
	DDRB |= (1 << LED);
	pwm_write(0);

	
	volatile uint8_t	mode = eeprom_read_byte(&MODE_P);
	volatile uint8_t	prev_mode = mode;
	volatile uint8_t	battery=0xFF;
	
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
	prev_mode=mode;
	
	while (1) {
		battery = map(adcread(), 145, 245, 0, 100);
		if(mode!=6){
			if(battery<10){
				if(mode!=0){
					mode=0;
					}else{
					pwm_write(0);
					_delay_ms(100);
				}
				}else{
				mode = prev_mode;
			}			
		}
		
		switch(mode){
			case 0:
			pwm_write(2);
			break;
			case 1:
			pwm_write(255/16);
			break;
			case 2:
			pwm_write(255/8);
			break;
			case 3:
			pwm_write(255/4);
			break;
			case 4:
			pwm_write(255/2);
			break;
			case 5:
			pwm_write(255);
			break;
			case 6:
			battery=battery/10;
			for(int i=0;i<=battery;i++){
				pwm_write(2);
				_delay_ms(200);
				pwm_write(0);
				_delay_ms(200);
			}
			break;			
			default:
			pwm_write(255/2);
			break;
		}		
		_delay_ms(1000);
	}
}