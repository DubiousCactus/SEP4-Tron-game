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

static Score score;


void communicate_serial_no_protocol(void *pvParameters)
{
	//Variables
	uint8_t data[] = "";

	_received_chars_queue = xQueueCreate(_COM_RX_QUEUE_LENGTH, (unsigned portBASE_TYPE) sizeof (uint8_t));
	init_com(_received_chars_queue);

	for(;;){

		/*Constantly checking data coming from the PC*/
		if (xQueueReceive(_received_chars_queue, &data, (TickType_t) 10)) {

			switch (data[0]) {
				case 0x41 :
					turn_player(&playerTwo, LEFT);		//A
					break;
				case 0x61 :
					turn_player(&playerTwo, LEFT);		//a
					break;
				case 0x57 :
					turn_player(&playerTwo, UP);		//W
					break;
				case 0x77 :
					turn_player(&playerTwo, UP);		//w
					break;
				case 0x44 :
					turn_player(&playerTwo, RIGHT);		//D
					break;
				case 0x64 :
					turn_player(&playerTwo, RIGHT);		//d
					break;
				case 0x53 :
					turn_player(&playerTwo, DOWN);		//S
					break;
				case 0x73 :
					turn_player(&playerTwo, DOWN);		//s
					break;
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


uint8_t* crc_check(uint8_t buffer[], uint8_t size) {
	int polynomial[9] = {1,1,1,0,0,0,0,0,1};
	int nbBits = size * 8 + STUFFED_ZEROS; //Length of frame: nb bytes x 8 bits + stuffed zeros
	int frame[nbBits];
	int n = 0, k = 0, l = 7;

	for(int i = (nbBits) - 1; i > 7; i--) { //From the last cell to the start of the stuffed zeros
		if(k == 8) { //If we converted the whole byte, go to the next
			n++;
			k = 0;
			l = 7;
		}

		frame[i] = (buffer[n] >> l) & 1; //Convert a byte, bit by bit
		k++;
		l--;
	}

	for(int i = 7; i >= 0; i--) { //Fill up the last part of the frame with the stuffed zeros
		frame[i] = 0;
	}


	bool xoring = false;
	int bitsToBeXored = 9;
	int xorBack = -1;
	int xoredBits[9] = {0};

	for(int i = nbBits - 1; i >= 0; i--) { //Through the whole frame
		if(frame[i] == 1 && !xoring) { //First 1 found after last division
			//Divide
			xoring = true; //Start xoring
		}

		if(xoring) { //While xoring
			int result = frame[i] ^ polynomial[bitsToBeXored - 1]; //XOR of frame's bit + polynomial's bit (synchronized)
			frame[i] = result; //Replace the frame's bit by the XORed value
			xoredBits[bitsToBeXored - 1] = result; //Fill up the result array

			if(result == 1 && xorBack == -1) { //If the FIRST result is a one, we set the index to XOR back
				xorBack = i;
			}

			bitsToBeXored--;
			if(bitsToBeXored == 0) { //We XORed 9 bits (the whole polynomial), reset everything
				bitsToBeXored = 9;
				xoring = false;
				i = xorBack; //Jump back to the first 1 from last result
				xorBack = -1;
			}
		}
	}

	return xoredBits;
}


void communicate_serial(void *pvParameters)
{
	//Variables
	uint8_t data[] = {0};
	ProtocolState state = IDLE;
	ProtocolState escapeCallback = IDLE:
	int payload_length = 0;
	int parsed_length = 0;
	int parsed_trailer = 0;
	int sequence_number = 0;
	uint8_t payload[MAX_PAYLOAD_LENGTH] = {0};
	uint8_t trailer[MAX_TRAILER_LENGTH] = {0};
	uint8_t frame_validated = false;

	_received_chars_queue = xQueueCreate(_COM_RX_QUEUE_LENGTH, (unsigned portBASE_TYPE) sizeof (uint8_t));
	init_com(_received_chars_queue);
		
	for(;;){

		data = {0};
		state = IDLE;
		payload_length = 0;
		parsed_length = 0;
		parsed_trailer = 0;
		sequence_number = 0;
		payload[MAX_PAYLOAD_LENGTH] = {0};
		trailer[MAX_TRAILER_LENGTH] = {0};
		frame_validated = false;
		
		while (!frame_validated) {

			switch(state) {
				
				case IDLE:
					xQueueReceive(_received_chars_queue, &data, (TickType_t) 10);
					if (data[0] != FLAG) {
						state = error;
					} else {
						state = HEADER;
					}
					break;
				case HEADER:
					xQueueReceive(_received_chars_queue, &data, (TickType_t) 10);
					if (data[0] == ESCAPE) {
						state = ESC;
						escapeCallback = HEADER;
					} else if (data[0] == FLAG) {
						state = ERROR;
					} else if (data[0] == DOT) {
						state = PAYLOAD;
					} else if (data[0] == COMMA) {
						//save sequence number
						sequence_number = data[0];	
					} else {
						//save payload length
						payload_length = data[0];
					}
					break;
				case PAYLOAD:
					if (parsed_length++ < payload_length) {
						xQueueReceive(_received_chars_queue, &data, (TickType_t) 10);
					} else if (data[0] == ESCAPE) {
						state = ESC;
						escapeCallback = PAYLOAD;
					} else if (data[0] == FLAG) {
						state = ERROR;
					} else if (payload_length == 0) {
						state = TRAILER;
					} else {
						payload[parsed_length] = data[0];
					}
					break;
				case TRAILER:
					xQueueReceive(_received_chars_queue, &data, (TickType_t) 10);
					if (data[0] == ESCAPE) {
						state = ESC;
						escapeCallback = TRAILER;
					} else if (data[0] == FLAG) {
						state = FRAME_VALIDATION;
					} else {
						trailer[parsed_trailer++] = data[0];
					}
					break;
				case ESC:
					state = escapeCallback;
					break;
				case ERROR:
					//send NACK
					break;
				case FRAME_VALIDATION:
					frame_validated = true;
					//send ACK
					break;
				case default:
					state = ERROR;
			}
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

		if(xSemaphoreTake(xGameStateSemaphore, (TickType_t) 10)){
			/* Create frame from gameState */
			for (int j = 0; j < 10; j++) { //Cumulate bits of each line
				for (int i = 0; i < 14; i++) { //For each column
					if (gameState[i][j] != 0) { //Add up
						frame_buffer[i]	+= (int) (pow(2,  j) + 0.5);
					}
				}
			}
			xSemaphoreGive(xGameStateSemaphore);
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

/* Returns true if collision */
uint8_t draw_players_lines(Player *player, int playerId)
  {
	uint8_t collision = false;	
	int from, to;

	for (int i = 0; i < (*player).turnsCount + 1; i++) {

		if (i < (*player).turnsCount) { //Processing every turn

			if ((*player).turns[i].x == (*player).turns[i + 1].x) { //Draw vertical line

				from = (*player).turns[i].y;
				to = (*player).turns[i + 1].y;

				if (from > to) {
					to = from;
					from = (*player).turns[i + 1].y;
				}

				//Draw line in gameState
				for (int j = from; j <= to; j++) {
					if ((playerId == 1 && gameState[(*player).turns[i].x][j] == 2)
							|| (playerId == 2 && gameState[(*player).turns[i].x][j] == 1)) //Collision !
						collision = true;
					else
						gameState[(*player).turns[i].x][j] = playerId;
				}

			} else if ((*player).turns[i].y == (*player).turns[i + 1].y) { //Draw horizontal line

				from = (*player).turns[i].x;
				to = (*player).turns[i + 1].x;

				if (from > to) {
					to = from;
					from = (*player).turns[i + 1].x;
				}

				//Draw line in gameState
				for (int j = from; j <= to; j++) {
					if ((playerId == 1 && gameState[j][(*player).turns[i].y] == 2)
						   || (playerId == 2 && gameState[j][(*player).turns[i].y] == 1)) //Collision !
						collision = true;
					else
						gameState[j][(*player).turns[i].y] = playerId;
				}

			}

		} else { //Processing the current position

			if ((*player).turns[i].x == (*player).x) { //Draw vertical line

				from = (*player).turns[i].y;
				to = (*player).y;

				if (from > to) {
					to = from;
					from = (*player).y;
				}

				//Draw line in gameState
				for (int j = from; j <= to; j++) {
					if ((playerId == 1 && gameState[(*player).x][j] == 2)
						   || (playerId == 2 && gameState[(*player).x][j] == 1)) //Collision !
						collision = true;
					else
						gameState[(*player).x][j] = playerId;
				}

			} else if ((*player).turns[i].y == (*player).y) { //Draw horizontal line

				from = (*player).turns[i].x;
				to = (*player).x;

				if (from > to) {
					to = from;
					from = (*player).x;
				}

				//Draw line in gameState
				for (int j = from; j <= to; j++) {
					if ((playerId == 1 && gameState[j][(*player).y] == 2) || (playerId == 2 && gameState[j][(*player).y] == 1)) //Collision !
						collision = true;
					else
						gameState[j][(*player).y] = playerId;
				}

			}

		}
	}

	return collision;
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
			if (xSemaphoreTake(xGameStateSemaphore, (TickType_t) 10) && xSemaphoreTake(xPlayerOneSemaphore, (TickType_t) 10) && xSemaphoreTake(xPlayerTwoSemaphore, (TickType_t) 10)) {
				for (int p = 1; p <= 2; p++) {
					/* Erase player */
					for (int i = 0; i < 14; i++)
					for (int j = 0; j < 10; j++)
					if ((p == 0 && gameState[i][j] == 1)
					|| (p == 1 && gameState[i][j] == 2))
					gameState[i][j] = 0;
			
			

					/* Move players in their current direction */
					if (p == 1) {
						collision = draw_players_lines(&playerOne, p);
						move_player(&playerOne);
					} else if (p == 2) {
						collision = draw_players_lines(&playerTwo, p);
						move_player(&playerTwo);
					}
					xSemaphoreGive(xGameStateSemaphore);
					xSemaphoreGive(xPlayerOneSemaphore);
					xSemaphoreGive(xPlayerTwoSemaphore);
				}
			}
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

	uint8_t right;
	uint8_t left;
	uint8_t up;
	uint8_t down;
	uint8_t pushed;
	Direction direction;
	uint8_t debounceCounter = 0;
	uint8_t debounceThreshold = 15;
	uint8_t turnPlayer = 0;
	uint8_t isPressing = 0;

	for (;;) {
		/*Constantly checking joystick state*/
		right	= !(PINC >> 1 & 0x01);
		left	= !(PINC >> 7 & 0x01);
		up		= !(PINC >> 6 & 0x01);
		down	= !(PINC >> 0 & 0x01);
		pushed  = !(PIND >> 3 & 0x01);

		if (down){
			direction = DOWN;
			turnPlayer = 1;
			isPressing = 1;
		} else if (right) {
			direction = RIGHT;
			turnPlayer = 1;
			isPressing = 1;
		} else if (up) {
			direction = UP;
			turnPlayer = 1;
			isPressing = 1;
		} else if (left) {
			direction = LEFT;
			turnPlayer = 1;
			isPressing = 1;
		} else if (pushed) {
			//TODO: pause game
			debounceCounter = 0;
		} else {
			isPressing = 0;
			//debounceCounter = 0;
		}

		if (turnPlayer && !isPressing && (++debounceCounter >= debounceThreshold)) {
			if (xSemaphoreTake(xPlayerOneSemaphore, (TickType_t) 10)) {
				turn_player(&playerOne, direction);
				turnPlayer = 0;
				debounceCounter = 0;
				xSemaphoreGive(xPlayerOneSemaphore);
			}
		}

		vTaskDelay(20);
	}
	
	vTaskDelete(NULL);
}



/* Changes the player's direction: will be applied in game_processing() */
void turn_player(Player *player, Direction direction)
{
	uint8_t validTurn = false, doTurn = false;

	if ((*player).direction != direction)
		doTurn = true;

	switch (direction) {
		case UP:
			if ((*player).direction == LEFT || (*player).direction == RIGHT) {
				(*player).direction = direction;
				validTurn = true;
			}
			break;
		case DOWN:
			if ((*player).direction == LEFT || (*player).direction == RIGHT) {
				(*player).direction = direction;
				validTurn = true;
			}
			break;
		case LEFT:
			if ((*player).direction == UP || (*player).direction == DOWN) {
				(*player).direction = direction;
				validTurn = true;
			}
			break;
		case RIGHT:
			if ((*player).direction == UP || (*player).direction == DOWN) {
				(*player).direction = direction;
				validTurn = true;
			}
			break;
	}

	if (doTurn && validTurn) { //New turn !
		(*player).turnsCount++;

		if ((*player).turnsCount < MAXTURNS && (*player).turns[(*player).turnsCount].x == -1) { //Free turn slot
			(*player).turns[(*player).turnsCount].x = (*player).x;
			(*player).turns[(*player).turnsCount].y =  (*player).y;
		} else {
			//TODO
		}

		move_player(player);
	}
}

/* Initialize the players' positions and turns */
void init_players()
{

	for (int i = 0; i < MAXTURNS + 1; i++) { //MAXTURNS + 1 because turn[0] is the init position, not a turn
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

	playerTwo.x = 13;
	playerTwo.y = 5;
	playerTwo.direction = LEFT;
	turn0;
	turn0.x = 13;
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

	init_players();

	BaseType_t taskReadJoystick = xTaskCreate(read_joystick, (const char*)"Read joystick", configMINIMAL_STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY, NULL);
	BaseType_t taskGameProcessing = xTaskCreate(game_processing, (const char*)"Game processing", configMINIMAL_STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY, NULL);
	BaseType_t taskMakeFrame = xTaskCreate(make_frame, (const char*)"Make frame", configMINIMAL_STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY, NULL );
	BaseType_t taskCommunicateSerialNoProto = xTaskCreate(communicate_serial_no_protocol, (const char*)"Communicate serial no protocol", configMINIMAL_STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY, NULL);

	// Start the display handler timer
	init_display_timer(handle_display);
	
	sei();
	
	//Start the scheduler
	vTaskStartScheduler();
	
	//Should never reach here
	while (1) {}
}


