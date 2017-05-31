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

// frame_buffer contains a bit pattern for each column in the display
static uint16_t frame_buffer[14] = {0};

//Shared variables
static int gameState[14][10] = {{0}};
static Player playerOne, playerTwo;
static uint8_t gameOver = 0;

//Semaphores for the shared variables
static SemaphoreHandle_t xGameStateSemaphore = NULL;
static SemaphoreHandle_t xPlayerOneSemaphore = NULL;
static SemaphoreHandle_t xPlayerTwoSemaphore = NULL;
static SemaphoreHandle_t  xGameOverSemaphore = NULL;

static Score score;

void communicate_serial(void *pvParameters)
{
	//Variables
	uint8_t data[] = "";

	_received_chars_queue = xQueueCreate(_COM_RX_QUEUE_LENGTH, (unsigned portBASE_TYPE) sizeof (uint8_t));
	init_com(_received_chars_queue);

	for(;;){

		/*Constantly checking data coming from the PC*/
		if (xQueueReceive(_received_chars_queue, &data, (TickType_t) 10)) {

			switch (data[0]) {
				case 0x41 : turn_player(&playerTwo, LEFT);	//A
				case 0x61 : turn_player(&playerTwo, LEFT);	//a
				case 0x57 : turn_player(&playerTwo, UP);		//W
				case 0x77 : turn_player(&playerTwo, UP);		//w
				case 0x44 : turn_player(&playerTwo, RIGHT);	//D
				case 0x64 : turn_player(&playerTwo, RIGHT);	//d
				case 0x53 : turn_player(&playerTwo, DOWN);	//S
				case 0x73 : turn_player(&playerTwo, DOWN);	//s
				default: com_send_bytes((uint8_t) data, 1);
			}
		}

		/*Constantly checking if game is over*/
		if (xSemaphoreTake(xGameOverSemaphore, (TickType_t) 10)) {
			/*Sending data to the PC when score is changed*/
			if (gameOver == true)
				com_send_bytes((uint8_t *)"GAME OVER\n", 10);
			
			xSemaphoreGive(xGameOverSemaphore);
		}

		vTaskDelay(20);
	}

	vTaskDelete(NULL);
}

/* TODO: protect frame_buffer with a mutex */
void make_frame(void *pvParameters)
{
	for(;;) {
		for (int j = 0; j < 10; j++) { //Cumulate bits of each line
			for (int i = 0; i < 14; i++) { //For each column
				frame_buffer[i]	= 0;
			}
		}

		/* Create frame from gameState */
		for (int j = 0; j < 10; j++) { //Cumulate bits of each line
			for (int i = 0; i < 14; i++) { //For each column
				if (gameState[i][j] != 0) { //Add up
					frame_buffer[i]	+= (int) (pow(2,  j) + 0.5);
				}
			}
		}

		vTaskDelay(50);
	}

	vTaskDelete(NULL);
}

void die()
{
	//TODO
	com_send_bytes("DEAD!\n", 6);
}

void move_player(Player* player)
{
	switch ((*player).direction) {
		case LEFT:
			if ((*player).x > 0)
				(*player).x--;
			break;
		case RIGHT:
			if ((*player).x < 14)
				(*player).x++;
			break;
		case UP:
			if ((*player).y > 0)
				(*player).y--;
			break;
		case DOWN:
			if ((*player).y < 10)
				(*player).y++;
			break;
	}

	//TODO: Figure out the front collision detection and death mechanism
}


/* TODO: protect gameState with a mutex */
void game_processing(void *pvParameters)
{
	/* Populate gameState from the players' positions and tracks:
	* Start at turn[i] to compare with turn[i - 1] for each player
	*/

	bool collision = false;

	for(;;) {

		while(!collision) {
			
			/* Erase player one */
			for (int i = 0; i < 14; i++)
				for (int j = 0; j < 10; j++)
					if (gameState[i][j] == 1)
						gameState[i][j] = 0;
			
			if (playerOne.turnsCount < 1) { //Didn't turn yet !

				if (playerOne.x == playerOne.turns[0].x) { //Vertical line

					int from, to;
					
					if (playerOne.y < playerOne.turns[0].y) {
						from = playerOne.y;
						to = playerOne.turns[0].y;
					} else {
						from = playerOne.turns[0].y;
						to = playerOne.y;
					}

					//Turn on LEDs for this line
					for (int j = from; j <= to; j++) {
						if (gameState[playerOne.turns[0].x][j] == 2) { //Collision with player two !!
							collision = 1;
						} else {
							gameState[playerOne.turns[0].x][j] = 1;
						}
					}

				} else if (playerOne.y == playerOne.turns[0].y) { //Horizontal line
	
					int from, to;
					
					if (playerOne.x < playerOne.turns[0].x) {
						from = playerOne.x;
						to = playerOne.turns[0].x;
					} else {
						from = playerOne.turns[0].x;
						to = playerOne.x;
					}
				
					for (int j = from; j <= to; j++) {
						if (gameState[j][playerOne.turns[0].y] == 2) { //Collision with player two !!
							collision = 1;
						} else {
							gameState[j][playerOne.turns[0].y] = 1;
						}
					}
				}
			} else {

				/* Draw player one and check collisions with player two */
				for (int i = 0; i < playerOne.turnsCount + 1; i++) {
					Turn turn;
					turn.x = playerOne.turns[i].x;
					turn.y = playerOne.turns[i].y;

					if (i == playerOne.turnsCount + 1) { //Current position, all turns were drawn
						turn.x = playerOne.x;
						turn.y = playerOne.y;
					}

					if (turn.x == playerOne.turns[i - 1].x) { //Vertical line

						int from, to;

						if (playerOne.turns[i - 1].y < turn.y) {
							from = playerOne.turns[i - 1].y;
							to = turn.y;
						} else {
							from = turn.y;
							to = playerOne.turns[i - 1].y;
						}

						//Turn on LEDs for this line
						for (int j = from; j <= to; j++) {
							if (gameState[turn.x][j] == 2) { //Collision with player two !!
								collision = 1;
							} else {
								gameState[turn.x][j] = 1;
							}
						}

					} else { //Horizontal line
						
						int from, to;

						if (playerOne.turns[i - 1].x < turn.x) {
							from = playerOne.turns[i - 1].x;
							to = turn.x;
						} else {
							from = turn.x;
							to = playerOne.turns[i - 1].x;
						}

						for (int j = from; j <= to; j++) {
							if (gameState[j][turn.y] == 2) { //Collision with player two !!
								collision = 1;
							} else {
								gameState[j][turn.y] = 1;
							}
						}
					}
				}
			}

			/* Move players in their current direction */
			move_player(&playerOne);
			//move_player(playerTwo);

			vTaskDelay(1000);
		}

		die();
	}

	vTaskDelete(NULL);
}


void read_joystick(void *pvParameters)
{
	//The parameters are not used
	( void ) pvParameters;

	uint8_t Right;
	uint8_t Left;
	uint8_t Up;
	uint8_t Down;
	uint8_t Pushed;
	Direction direction;
	uint8_t debounceCounter = 0;
	uint8_t debounceThreshold = 15;
	uint8_t turnPlayer = 0;
	uint8_t isPressing = 0;

	for (;;) {
		/*Constantly checking joystick state*/
		Right	= !(PINC >> 1 & 0x01);
		Left	= !(PINC >> 7 & 0x01);
		Up		= !(PINC >> 6 & 0x01);
		Down	= !(PINC >> 0 & 0x01);
		Pushed  = !(PIND >> 3 & 0x01);

		if (Down){
			direction = DOWN;
			turnPlayer = 1;
			isPressing = 1;
		} else if (Right) {
			direction = RIGHT;
			turnPlayer = 1;
			isPressing = 1;
		} else if (Up) {
			direction = UP;
			turnPlayer = 1;
			isPressing = 1;
		} else if (Left) {
			direction = LEFT;
			turnPlayer = 1;
			isPressing = 1;
		} else if (Pushed) {
			//TODO: pause game
			debounceCounter = 0;
		} else {
			isPressing = 0;
			//debounceCounter = 0;
		}

		if (turnPlayer && !isPressing && (++debounceCounter >= debounceThreshold)) {
			com_send_bytes("move", 4);
			turn_player(&playerOne, direction);
			turnPlayer = 0;
			debounceCounter = 0;
		}

		vTaskDelay(20);
	}
	
	vTaskDelete(NULL);
}



/* Changes the player's direction: will be applied in game_processing() */
void turn_player(Player *player, Direction direction)
{

	if ((*player).direction != direction) { //New turn !
		(*player).turnsCount++;

		if ((*player).turnsCount < MAXTURNS && (*player).turns[(*player).turnsCount].x == -1) { //Free turn slot
			(*player).turns[(*player).turnsCount].x = (*player).x;
			(*player).turns[(*player).turnsCount].y = (*player).y;
		} else {
			//TODO
		}
	}

	switch (direction) {
		case UP:
			if ((*player).direction == LEFT || (*player).direction == RIGHT)
				(*player).direction = direction;
			break;
		case DOWN:
			if ((*player).direction == LEFT || (*player).direction == RIGHT)
				(*player).direction = direction;
			break;
		case LEFT:
			if ((*player).direction == UP || (*player).direction == DOWN)
				(*player).direction = direction;
			break;
		case RIGHT:
			if ((*player).direction == UP || (*player).direction == DOWN)
				(*player).direction = direction;
			break;
	}
}

/* Initialize the players' positions and turns */
void init_players()
{

	for (int i = 0; i < MAXTURNS + 1; i++) {
		playerOne.turns[i].x = -1;
		playerOne.turns[i].y = -1;
		playerTwo.turns[i].x = -1;
		playerTwo.turns[i].y = -1;
	}

	playerOne.turnsCount = 0;
	playerTwo.turnsCount = 0;

	playerOne.x = 0;
	playerOne.y = 5;
	playerOne.direction = RIGHT;
	Turn turn0;
	turn0.x = 0;
	turn0.y = 5;
	playerOne.turns[0] = turn0;

	playerTwo.x = 10;
	playerTwo.y = 5;
	playerTwo.direction = LEFT;
	turn0;
	turn0.x = 10;
	turn0.y = 5;
	playerTwo.turns[0] = turn0;
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

	/* Semaphores creation */
	xGameStateSemaphore = xSemaphoreCreateMutex();
	xPlayerOneSemaphore = xSemaphoreCreateMutex();
	xPlayerTwoSemaphore = xSemaphoreCreateMutex();
	xGameOverSemaphore = xSemaphoreCreateMutex();

	init_players();

	BaseType_t taskReadJoystick = xTaskCreate(read_joystick, (const char*)"Read joystick", configMINIMAL_STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY, NULL);
	BaseType_t taskGameProcessing = xTaskCreate(game_processing, (const char*)"Game processing", configMINIMAL_STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY, NULL);
	BaseType_t taskMakeFrame = xTaskCreate(make_frame, (const char*)"Make frame", configMINIMAL_STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY, NULL );


	// Start the display handler timer
	init_display_timer(handle_display);
	
	sei();
	
	//Start the scheduler
	vTaskStartScheduler();
	
	//Should never reach here
	while (1) {}
}


