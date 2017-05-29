#include "stdint.h"
#include "stdio.h"
#include "math.h"
#include "stdlib.h"
#include <stdbool.h>

#define STUFFED_ZEROS 8 

uint8_t crc_check(uint8_t buffer[], uint8_t size);

int main(char argc, char *argv[]) {

	uint8_t buffer[6] = {0x34, 0x47, 0xF2, 0xA7, 0x67, 0xDE};
	uint8_t crc = crc_check(buffer, 6);

	printf("CRC: 0x%X\n", crc);

	return 0;
}

uint8_t crc_check(uint8_t buffer[], uint8_t size) {
	int polynomial[9] = {1,1,1,0,0,0,0,0,1};
	int nbBits = size * 8 + STUFFED_ZEROS; //Length of frame: nb bytes x 8 bits + stuffed zeros
	int frame[nbBits];
	int n = 0, k = 0, l = 7;

	printf("Size: %d \nBits:\n", nbBits);
	printf("\nByte number: %d -> ", n);
	for(int i = (nbBits) - 1; i > 7; i--) { //From the last cell to the start of the stuffed zeros
		if(k == 8) { //If we converted the whole byte, go to the next
			n++;
			k = 0;
			l = 7;
			printf("\nByte number: %d -> ", n);
		}
		frame[i] = (buffer[n] >> l) & 1; //Convert a byte, bit by bit
		k++;
		l--;
		printf("%d ", frame[i]);
	}

	printf("\nPolynomial empty bits: ");
	for(int i = 7; i >= 0; i--) { //Fill up the last part of the frame with the stuffed zeros
		frame[i] = 0;
		printf("%d ", frame[i]);
	}

	printf("Frame build !\nFrame -> ");
	for(int i = nbBits - 1; i >= 0; i--) {
		printf("%d", frame[i]);
	}

	printf("\nPolynomial -> ");
	for(int i = 8; i >= 0; i--) {
		printf("%d", polynomial[i]);
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

	printf("\nResult -> ");
	for(int i = 8; i >= 0; i--) {
		printf("%d", xoredBits[i]);
	}

	printf("\n");
}