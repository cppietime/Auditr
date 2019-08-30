#ifndef _H_FFT
#define _H_FFT

#include <complex.h>
#include <math.h>
#include <stdint.h>

void fft(complex double*, complex double*, int, int, int);
void ifft(complex double*, complex double*, int);

#endif