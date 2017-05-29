#include "stdint.h"
#include "stdio.h"
#include "math.h"
#include "stdlib.h"

uint8_t calc_checksum(uint8_t buffer[], uint8_t size);

int main(char argc, char *argv[]) {

	uint8_t buffer[6] = {0x34, 0x47, 0xF2, 0xA7, 0x67, 0xDE};

	uint8_t chcksum = calc_checksum(buffer, 6);

	printf("Checksum: 0x%X\n", chcksum);


	uint8_t bufferAndChecksum[7] = {0};
	for(int i = 0; i < 6; i++)
		bufferAndChecksum[i] = buffer[i];

	bufferAndChecksum[6] = chcksum;

	printf("Reversing checksum: ");
	uint8_t result = calc_checksum(bufferAndChecksum, 7);
	printf("0x%X\n", result);

	return 0;
}


uint8_t calc_checksum(uint8_t buffer[], uint8_t size) {
	uint16_t sum = 0, modulo = 0;

	for(int i = 0; i < size; i++) {
		sum += buffer[i];
	}

	printf("Sum: 0x%X \n", sum);

	modulo = sum % (uint16_t)pow(2, 8);

	printf("Modulo: 0x%X \n", modulo);

	return (~modulo) + 1;
}
