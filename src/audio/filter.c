#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include "filter.h"
#include "wav.h"
#include "fft.h"

void rfunc_free(rfunc* func){
	if(func==NULL)
		return;
	if(func->coefs!=NULL)
		free(func->coefs);
	free(func);
}

void rfunc_print(rfunc* func){
	int i;
	for(i=0; i<func->order+1; i++){
		printf("%f*z^%d",func->coefs[i],i);
		if(i<func->order)printf("+");
	}
	printf("\n");
}

rfunc* rfunc_mul(rfunc* a, rfunc* b){
	rfunc* ret = malloc(sizeof(rfunc));
	ret->order = a->order + b->order;
	ret->coefs = calloc(ret->order+1, sizeof(double));
	int i,j;
	for(i=0; i<a->order+1; i++){
		for(j=0; j<b->order+1; j++){
			ret->coefs[i+j] += a->coefs[i] * b->coefs[j];
		}
	}
	return ret;
}

rfunc* rfunc_conj(double real, double imag){
	rfunc* ret = malloc(sizeof(rfunc));
	ret->order = 2;
	ret->coefs = calloc(ret->order+1, sizeof(double));
	ret->coefs[0] = real * real + imag* imag;
	ret->coefs[1] = -2 * real;
	ret->coefs[2] = 1;
	return ret;
}

rfunc* for_roots(double* roots, int num){
	rfunc* ret = rfunc_conj(roots[0],roots[1]);
	int i;
	for(i=1; i<num; i++){
		rfunc* root = rfunc_conj(roots[i*2],roots[i*2+1]);
		rfunc* prod = rfunc_mul(ret,root);
		rfunc_free(root);
		rfunc_free(ret);
		ret = prod;
	}
	return ret;
}

complex double rfunc_calc(rfunc* func, complex double x){
	if(func==NULL)
		return 1;
	complex double ret = 0;
	complex double factor = 1;
	int i;
	for(i=0; i<func->order+1; i++){
		ret += factor * func->coefs[i];
		factor *= x;
	}
	return ret;
}

rfunc* all_poles(double srate, double mag, double* freqs, int num){
	double* poles = malloc(sizeof(double)*num*2);
	int i;
	for(i=0; i<num; i++){
		complex double pole = cexp(2*M_PI*freqs[i]/srate*I)*mag;
		poles[i*2] = creal(pole);
		poles[i*2+1] = cimag(pole);
	}
	rfunc* ret = for_roots(poles,num);
	rfunc_divhead(ret);
	return ret;
}

rfunc* point_poles(double srate, double* mags, double* freqs, int num){
	double* poles = malloc(sizeof(double)*num*2);
	int i;
	for(i=0; i<num; i++){
		complex double pole = mags[i]*cexp(2*M_PI*freqs[i]*I/srate);
		poles[i*2] = creal(pole);
		poles[i*2+1] = cimag(pole);
	}
	rfunc* ret = for_roots(poles,num);
	rfunc_divhead(ret);
	return ret;
}

double rfunc_max_response(rfunc* top, rfunc* den, double srate, double* freqs, int num){
	double ret = 0;
	int i;
	for(i=0; i<num; i++){
		complex double unit = cexp(freqs[i]/srate*2*M_PI*I);
		complex double rnum, rdem;
		if(top!=NULL)
			rnum = rfunc_calc(top,unit);
		else
			rnum = cpow(unit,den->order);
		rdem = rfunc_calc(den,unit);
		complex double response = rnum/rdem;
		double val = cabs(response);
		if(val>ret)
			ret = val;
	}
	return ret;
}

void rfunc_divhead(rfunc* func){
	int i;
	double norm = func->coefs[func->order];
	for(i=func->order-1; i>=0; i--){
		func->coefs[i] /= norm;
	}
}

void rfunc_normalize(rfunc* func, double fac){
	int i;
	for(i=0; i<func->order+1; i++){
		func->coefs[i]/=fac;
	}
}

void filter(double* dst, double* src, int len, rfunc* num, rfunc* den){
	int i,j;
	for(i=0; i<len; i++){
		dst[i]=0;
		for(j=0; j<num->order+1; j++){
			if(i-j>0)
				dst[i] += src[i-j] * num->coefs[num->order-j];
		}
		if(den!=NULL){
			for(j=0; j<den->order; j++){
				if(i-j-1>0)
					dst[i] -= dst[i-j-1] * den->coefs[den->order-1-j];
			}
		}
	}
}

void filter_in_place(double* data, int len, rfunc* num, rfunc* den){
	int ring = num->order;
	int ptr = 0;
	double* buffer = NULL;
	if(ring>0)
		buffer = calloc(ring,sizeof(double));
	int i,j;
	for(i=0; i<len; i++){
		double point = data[i] * num->coefs[num->order];
		for(j=0; j<ring; j++){
			point += buffer[(ptr-j+ring)%ring] * num->coefs[num->order-j-1];
		}
		if(den!=NULL){
			for(j=0; j<den->order; j++){
				if(i-j-1>0)
					point -= data[i-j-1] * den->coefs[den->order-1-j];
			}
		}
		if(ring>0){
			ptr = (ptr+1)%ring;
			buffer[ptr] = data[i];
		}
		data[i] = point;
	}
	if(ring>0)
		free(buffer);
}

// int main(){
	// srand(time(NULL));
	// int i;
	// int srate = 44100;
	// make_vowels(srate);
	// int secs = 10;
	// int length = secs*srate;
	// double* samps = malloc(sizeof(double)*length);
	// phoneme phone = {1,100,5,.25,sawtooth};
	// for(i=0; i<40; i++){
		// int note = rand()%40;
		// double freq = 55*pow(2,note/12.);
		// phone.frequency=freq;
		// play_phoneme(V_I,C_L,&phone,srate,samps+length/40*i,samps+length);
	// }
	// wavhead header;
	// header.channels=1;
	// header.sampleRate=srate;
	// header.bps=16;
	// header.dataSize = 2*length;
	// validate_header(&header);
	// int16_t* data = malloc(sizeof(int16_t)*length);
	// int max = 0;
	// for(i=0; i<length; i++){
		// data[i] = samps[i]*32767;
		// if(data[i]>max)max=data[i];
	// }
	// printf("Max: %d\n",max);
	// FILE* output = fopen("train.wav","wb");
	// fwrite(&header,sizeof(header),1,output);
	// fwrite(data,2,length,output);
	// fclose(output);
	// free(samps);
	// free(data);
	// clear_vowels();
	// return 0;
// }