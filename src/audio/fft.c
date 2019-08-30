#include <complex.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fft.h"

void fft(complex double* in, complex double* out, int istr, int ostr, int size){
	if(size==1){
		*out = *in;
		return;
	}
	if(size&1){
		int i,j;
		for(i=0; i<size; i++){
			out[i*ostr] = 0;
			for(j=0; j<size; j++){
				out[i*ostr] += in[j*istr] * cexp(-2.0*M_PI*i*j/size*I);
			}
		}
		return;
	}
	fft(in,out,istr*2,ostr,size/2);
	fft(in+istr,out+ostr*size/2,istr*2,ostr,size/2);
	int i;
	for(i=0; i<size/2; i++){
		double complex tmp = out[i*ostr];
		double complex twid = cexp(-2.0*M_PI*i/size*I) * out[ostr*(i+size/2)];
		out[i*ostr] = tmp+twid;
		out[(i+size/2)*ostr] = tmp-twid;
	}
}

void ifft(complex double* in, complex double* out, int size){
	int i;
	for(i=0; i<size; i++){
		double re = creal(in[i]);
		double im = cimag(in[i]);
		in[i] = im + re*I;
	}
	fft(in,out,1,1,size);
	for(i=0; i<size; i++){
		double re = creal(in[i]);
		double im = cimag(in[i]);
		in[i] = im + re*I;
		re = creal(out[i]);
		im = cimag(out[i]);
		out[i] = (im + re*I)/size;
	}
}