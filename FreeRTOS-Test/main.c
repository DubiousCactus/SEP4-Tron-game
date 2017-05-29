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
#include <math.h>
#include <stdbool.h>

#include "src/board/board.h"

//Custom types
#include "types.h"

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

	bool collision = false;

	while(!collision) {
		
		/* Erase player one */
		for (int i = 0; i < 14; i++)
			for (int j = 0; j < 10; j++)
				if (gameState[i][j] == 1)
					gameState[i][j] = 0;

		if (sizeof(playerOne.turns) < 2) { //Didn't turn yet !

			if (playerOne.x == playerOne.turns[0].x) { //Vertical line

				//Turn on LEDs for this line
				for (int j = playerOne.y; j <= playerOne.turns[0].y; j++) {
					if (gameState[playerOne.turns[0].x][j] == 2) //Collision with player two !!
						collision = true;
					else
						gameState[playerOne.turns[0].x][j] = 1;
				}

			} else { //Horizontal line
				
				for (int j = playerOne.x; j <= playerOne.turns[0].x; j++) {
					if (gameState[j][playerOne.turns[0].y] == 2) //Collision with player two !!
						collision = true;
					else
						gameState[j][playerOne.turns[0].y] = 1;
				}
			}
		} else {

			/* Draw player one and check collisions with player two */
			for (int i = 1; i < sizeof(playerOne.turns); i++) {
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
		}

		/* Erase player two */
		for (int i = 0; i < 14; i++)
			for (int j = 0; j < 10; j++)
				if (gameState[i][j] == 2)
					gameState[i][j] = 0;

		if (sizeof(playerTwo.turns) < 2) { //Didn't turn yet !

			if (playerTwo.x == playerTwo.turns[0].x) { //Vertical line

				//Turn on LEDs for this line
				for (int j = playerTwo.y; j <= playerTwo.turns[0].y; j++) {
					if (gameState[playerTwo.turns[0].x][j] == 2) //Collision with player two !!
						collision = true;
					else
						gameState[playerTwo.turns[0].x][j] = 1;
				}

			} else { //Horizontal line
				
				for (int j = playerTwo.x; j <= playerTwo.turns[0].x; j++) {
					if (gameState[j][playerTwo.turns[0].y] == 2) //Collision with player two !!
						collision = true;
					else
						gameState[j][playerTwo.turns[0].y] = 1;
				}
			}
		} else {

			/* Draw player two and check collisions with player one */
			for (int i = 1; i < sizeof(playerTwo.turns); i++) {
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
		}

		/* Move players in their current direction */
		switch (playerOne.direction) {
			case LEFT:
				playerOne.x--;
				break;
			case RIGHT:
				playerOne.x++;
				break;
			case UP:
				playerOne.y--;
				break;
			case DOWN:
				playerOne.y++;
				break;
		}

		switch (playerTwo.direction) {
			case LEFT:
				playerTwo.x--;
				break;
			case RIGHT:
				playerTwo.x++;
				break;
			case UP:
				playerTwo.y--;
				break;
			case DOWN:
				playerTwo.y++;
				break;
		}
	}
}

bool correct_arrow_key(char data_received[])
{
	switch (data_received[0])
	{
		case 0x41 : return true;	//A
		case 0x61 : return true;	//a
		case 0x57 : return true;	//W
		case 0x77 : return true;	//w
		case 0x44 : return true;	//D
		case 0x64 : return true;	//d
		case 0x53 : return true;	//S
		case 0x73 : return true;	//s
		default  : return false;

	}
}

void communicate_serial(void *pvParameters)
{
	
	//Variables
	uint8_t data[] = "";

	_received_chars_queue = xQueueCreate(_COM_RX_QUEUE_LENGTH, (unsigned portBASE_TYPE) sizeof (uint8_t));
	init_com(_received_chars_queue);	

	while (1) {
		//Check data sent from pc
		if (xQueueReceive(_received_chars_queue, &data, (TickType_t) 10)) {

			//Received data from pc - check if it's a correct arrow key
			if (correct_arrow_key(&data) == true){
				//Goodie, this is a correct arrow, move Player 2 avatar.
			}
		}
	}

}

void read_joystick(void *pvParameters)
{

	while (1) {
		uint8_t Right	= PINC>>1 & 0x01;
		uint8_t Left	= PINC>>7 & 0x01;
		uint8_t Up		= PINC>>6 & 0x01;
		uint8_t Down	= PINC>>0 & 0x01;
		uint8_t Pushed  = PIND>>3 & 0x01;
	
		//Waiting for joystick to be pressed
		//Down
		if (Down == 0){		
			com_send_bytes((uint8_t *)"Down\n", 5);
			turn_player(playerOne, Direction.DOWN);

		//Right
		} else if (Right == 0) {
			com_send_bytes((uint8_t *)"Right\n", 6);
			turn_player(playerOne, Direction.RIGHT);
		
		//Up
		} else if (Up == 0) {
			com_send_bytes((uint8_t *)"Up\n", 3);
			turn_player(playerOne, Direction.UP);

		//Left
		} else if (Left == 0) {
			com_send_bytes((uint8_t *)"Left\n", 5);
			turn_player(playerOne, Direction.LEFT);
		
		//Push
		} else if (Pushed == 0) {
			com_send_bytes((uint8_t *)"Pause\n", 6);

		} else {

		}
	}
}

void turn_player(Position player, Direction direction)
{

	switch (direction) {
		case UP:
			if (player.direction == Direction.LEFT || player.direction == Direction.RIGHT)
				player.direction = direction;
			break;
		case DOWN:
			if (player.direction == Direction.LEFT || player.direction == Direction.RIGHT)
				player.direction = direction;
			break;
		case LEFT:
			if (player.direction == Direction.UP || player.direction == Direction.DOWN)
				player.direction = direction;
			break;
		case RIGHT:
			if (player.direction == Direction.UP || player.direction == Direction.DOWN)
				player.direction = direction;
			break;
	}
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


