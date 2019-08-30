#ifndef _WAV_H
#define _WAV_H

#include <complex.h>

typedef struct __attribute__((__packed__)) _wavhead{
	int8_t chunkID[4];
	uint32_t chunkSize;
	int8_t fmt[4];
	int8_t subID[4];
	uint32_t subSize;
	uint16_t subFmt;
	uint16_t channels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t block;
	uint16_t bps;
	int8_t dataID[4];
	uint32_t dataSize;
} wavhead;

void readWave(wavhead*, void**, FILE*);
void toDoubles(wavhead*, void*, complex double**, char);
void validate_header(wavhead*);

#endif