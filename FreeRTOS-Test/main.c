#include <avr/sfr_defs.h>
#include <avr/io.h>
#include <avr/interrupt.h>

// FfreeRTOS Includes
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <queue.h>
#include <semphr.h>

//CNC 20170525 Added to have booleans
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "src/board/board.h"

//Custom types
#include "tags.h"

static const uint8_t _COM_RX_QUEUE_LENGTH = 30;

static QueueHandle_t _received_chars_queue = NULL;
static SemaphoreHandle_t  xMutexReceivedData = NULL;

// frame_buffer contains a bit pattern for each column in the display
static uint16_t frame_buffer[14] = {0};

static int gameState[14][10] = {{0}};
static Position playerOne, playerTwo;
static Score score;


/* TODO: protect frame_buffer with a mutex */
void make_frame(void *pvParameters)
{
	/* Create frame from gameState */
	for (int i = 0; i < 14; i++) { //For each column
		for (int j = 0; j < 10; j++) { //Cumulate bits of each line
			if (gameState[i][j] != 0) { //Add up
				frame_buffer[i]	+= pow(2, j);
			}
		}
	}
}


/* TODO: protect gameState with a mutex */
void game_processing(void *pvParameters)
{
	/* Populate gameState from the players' positions and tracks:
 	 * Start at turn[i] to compare with turn[i - 1] for each player
	 */

	/* Erase player one */
	for (int i = 0; i < 14; i++)
		for (int j = 0; j < 10; j++)
			if (gameState[i][j] == 1)
				gameState[i][j] = 0;

	/* Draw player one and check collisions with player two */
	for (int i = 1; i < playerOne.turns.length; i++) {
		if (playerOne.turns[i].x == playerOne.turns[i - 1].x) { //Vertical line

			//Turn on LEDs for this line
			for (int j = playerOne.turns[i - 1].y; j <= playerOne.turns[i].y; j++) {
				if (gameState[playerOne.turns[i].x][j] == 2) //Collision with player two !!
					collision = true;
				else
					gameState[playerOne.turns[i].x][j] = 1;
			}

		} else { //Horizontal line
			
			for (int j = playerOne.turns[i - 1].x; j <= playerOne.turns[i].x; j++) {
				if (gameState[j][playerOne.turns[i].y] == 2) //Collision with player two !!
					collision = true;
				else
					gameState[j][playerOne.turns[i].y] = 1;
			}
		}
	}

	/* Erase player two */
	for (int i = 0; i < 14; i++)
		for (int j = 0; j < 10; j++)
			if (gameState[i][j] == 2)
				gameState[i][j] = 0;

	/* Draw player two and check collisions with player one */
	for (int i = 1; i < playerTwo.turns.length; i++) {
		if (playerTwo.turns[i].x == playerTwo.turns[i - 1].x) { //Vertical line

			//Turn on LEDs for this line
			for (int j = playerTwo.turns[i - 1].y; j <= playerTwo.turns[i].y; j++) {
				if (gameState[playerTwo.turns[i].x][j] == 2) //Collision with player one !!
					collision = true;
				else
					gameState[playerTwo.turns[i].x][j] = 1;
			}

		} else { //Horizontal line
			
			for (int j = playerTwo.turns[i - 1].x; j <= playerTwo.turns[i].x; j++) {
				if (gameState[j][playerTwo.turns[i].y] == 2) //Collision with player one !!
					collision = true;
				else
					gameState[j][playerTwo.turns[i].y] = 1;
			}
		}
	}

	/* Calculate score */

}

void communicate_serial(void *pvParameters)
{

}

void read_joystick(void *pvParameters)
{

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
	
	// Start the display handler timer
	init_display_timer(handle_display);
	
	sei();
	
	//Start the scheduler
	vTaskStartScheduler();
	
	//Should never reach here
	while (1) {}
}


