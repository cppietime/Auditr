#ifndef _H_FFT
#define _H_FFT

#include <complex.h>
#include <math.h>
#include <stdint.h>

void fft(complex double*, complex double*, int, int, int);
void ifft(complex double*, complex double*, int);

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