#include<stdio.h>

int main()
{
    /* first C program */
    uint8_t complementChecksumBuffer[6];
	complementChecksumBuffer[0] = 0x34;
	complementChecksumBuffer[1] = 0x47;
	complementChecksumBuffer[2] = 0xF2;
	complementChecksumBuffer[3] = 0xA7;
	complementChecksumBuffer[4] = 0x67;
	complementChecksumBuffer[5] = 0xDE;
	
	uint8_t result = calc_checksum(complementChecksumBuffer[], 6);
	
    getch(); //Use to get one character input from user, and it will not be printed on screen.
    return 0;
}

uint8_t calc_checksum(uint8_t buffer[], uint8_t size)
{
	
}