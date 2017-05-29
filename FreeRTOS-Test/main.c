#include <avr/sfr_defs.h>
#include <avr/io.h>
#include <avr/interrupt.h>

// FreeRTOS Includes
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <queue.h>
#include <semphr.h>

//Our Includes
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "src/board/board.h"

static const uint8_t _COM_RX_QUEUE_LENGTH = 30;

static QueueHandle_t _received_chars_queue = NULL;
static SemaphoreHandle_t  xMutexReceivedData = NULL;

// frame_buffer contains a bit pattern for each column in the display
static uint16_t frame_buffer[14] = {0};


void refresh_screen(void *pvParameters)
{
	/* Create frame from gameState */
}


void game_processing(void *pvParameters)
{
	/* Populate gameState from the players' positions and tracks */


	/* Calculate collisions */


	/* Calculate score */

}

void communicate_serial(void *pvParameters)
{

}

void read_joystick(void *pvParameters)
{
	while (1) {
		//Waiting for joystick to be pressed
		if (PINC & 0b11001000){
			//Up
			com_send_bytes((uint8_t *)"Up\n", 4);
			} else if (PINC & 0b00101000) {
			//Left
			com_send_bytes((uint8_t *)"Left\n", 6);
			} else if (PINC & 0b10011000) {
			//Down
			com_send_bytes((uint8_t *)"Down\n", 6);
			} else if (PINC & 0b01001110) {
			//Right
			com_send_bytes((uint8_t *)"Right\n", 7);
			} else if (PIND3 & 0b00001100) {
			//Push - Pause mode
			com_send_bytes((uint8_t *)"Pause\n", 7);
			} else {
			
		}
		vTaskDelay(500);
	}
	//Left  -> 0b10000000
	//Up    -> 0b01000000
	//Down  -> 0b00000001
	//Right -> 0b00000010
	//Push  -> 0b00001100

}

// Prepare shift register setting SER = 1
void prepare_shiftregister()
{
	// Set SER to 1
	PORTD |= _BV(PORTD2);
}

// clock shift-register
void clock_shift_register_and_prepare_for_next_col()
{
	// one SCK pulse
	PORTD |= _BV(PORTD5);
	PORTD &= ~_BV(PORTD5);
	
	// one RCK pulse
	PORTD |= _BV(PORTD4);
	PORTD &= ~_BV(PORTD4);
	
	// Set SER to 0 - for next column
	PORTD &= ~_BV(PORTD2);
}

// Load column value for column to show
void load_col_value(uint16_t col_value)
{
	PORTA = ~(col_value & 0xFF);
	
	// Manipulate only with PB0 and PB1
	PORTB |= 0x03;
	PORTB &= ~((col_value >> 8) & 0x03);
}

//-----------------------------------------
void handle_display(void)
{
	static uint8_t col = 0;
	
	if (col == 0)
		prepare_shiftregister();
	
	load_col_value(frame_buffer[col]);
	
	clock_shift_register_and_prepare_for_next_col();
	
	// count column up - prepare for next
	if (col++ > 13)
		col = 0;
}

//-----------------------------------------
void vApplicationIdleHook( void )
{
	//
}

//-----------------------------------------
int main(void)
{
	
	init_board();
	
	// Shift register Enable output (G=0)
	PORTD &= ~_BV(PORTD6);

	xTaskCreate(read_joystick, (const char*)"Read joystick", configMINIMAL_STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY, NULL);
	
	// Start the display handler timer
	init_display_timer(handle_display);
	
	sei();
	
	//Start the scheduler
	vTaskStartScheduler();
	
	//Should never reach here
	while (1) {}
}


