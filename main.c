/*
 Beschaltung: Pin 1 offen (Reset)
 Pin 2 Ausgang fuer LED
 Pin 3 Ausgang fuer LED
 Pin 4 Ground, Masse
 Pin 5 Ausgang fuer LED
 Pin 6 Ausgang fuer LED
 Pin 7 Taster
 Pin 8 Vcc, +3,7V
 */

#include <inttypes.h>
#include <avr/io.h>
#include <avr/iotn85.h>
#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <util/delay.h>


//-----------------------------------------------------------------------------
volatile uint8_t ucState = 0;


//-----------------------------------------------------------------------------
// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms,6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void setup_watchdog(uint8_t ii) {
	uint8_t bb;
	if (ii > 9)
		ii = 9;
	bb = (ii + 0x18) & 0x27;
	bb |= (1 << WDCE);
	MCUSR &= ~(1 << WDRF);
	WDTCR |= (1 << WDCE) | (1 << WDE);
	WDTCR = bb;
	WDTCR |= _BV(WDIE);
}

//-----------------------------------------------------------------------------
void system_sleep() {
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_mode();
}

//-----------------------------------------------------------------------------
int delay_seconds_by_watchdog(uint16_t sec) {
	uint16_t t1;
	uint8_t t2; //t2 ist die tatsaechliche Watchdog-Zeit in Sekunden
	uint8_t t3; //t3 ist der zugehoerige Parameter hierzu
	uint8_t ucOrigState = ucState;
	t1 = sec;
	while (t1 > 0) {
		if (t1 > 7) {
			t3 = 9;
			t2 = 8;
		} else if (t1 > 3) {
			t3 = 8;
			t2 = 4;
		} else if (t1 > 1) {
			t3 = 7;
			t2 = 2;
		} else {
			t3 = 6;
			t2 = 1;
		};
		setup_watchdog(t3);
		system_sleep();
		if (ucState != ucOrigState) {
			return (0);
		}
		t1 -= t2;
	}
	return (1);
}

//-----------------------------------------------------------------------------
ISR(WDT_vect) {
}

//-----------------------------------------------------------------------------
ISR(INT0_vect) {
	if ((PINB & (1 << PB2)) != 0) {
		switch (ucState) {
		case 0:
			ucState = 1;
			break;
		case 2:
			ucState = 3;
			break;
		}
	}
}

//-----------------------------------------------------------------------------
int main(void) {
	MCUCR &= ~((1<<ISC01)|(1<<ISC00));
	GIMSK |= (1<<INT0);

	PORTB |= (1<<PB2);

	sei();
	for (;;) {

		switch (ucState) {
		case 0:
			setup_watchdog(9);
			system_sleep();
//			DDRB = (1 << PB0) | (1 << PB1) | (1 << PB3) | (1 << PB4);
//			PORTB |= (1 << PB0) | (1 << PB1) | (1 << PB3) | (1 << PB4);
//			_delay_ms(40);
//			PORTB &= ~((1 << PB0) | (1 << PB1) | (1 << PB3) | (1 << PB4));
//			DDRB = 0;
			break;
		case 1:
			DDRB = (1 << PB0) | (1 << PB1) | (1 << PB3) | (1 << PB4);
			PORTB |= (1 << PB0) | (1 << PB1) | (1 << PB3) | (1 << PB4);
			GIMSK &= ~(1<<INT0);
			delay_seconds_by_watchdog(1);
			GIMSK |= (1<<INT0);
			ucState = 2;
			break;
		case 2:
			delay_seconds_by_watchdog(120 * 60);
			ucState = 3;
			break;
		case 3:
			PORTB &= ~((1 << PB0) | (1 << PB1) | (1 << PB3) | (1 << PB4));
			DDRB = 0;
			GIMSK &= ~(1<<INT0);
			delay_seconds_by_watchdog(1);
			GIMSK |= (1<<INT0);
			ucState = 0;
			break;
		}
	}
	return 0;
}

