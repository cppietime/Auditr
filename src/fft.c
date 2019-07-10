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

void readWave(wavhead* head, void** data, FILE* file){
	fread(head, sizeof(wavhead), 1, file);
	*data = malloc(head->dataSize);
	fread(*data, 1, head->dataSize, file);
}

void toDoubles(wavhead* head, void* data, complex double** out, char channel){
	int samps = head->dataSize/head->block;
	*out = malloc(sizeof(complex double)*samps);
	int i;
	for(i=0; i<samps; i++){
		int offset = i*head->block;
		double val = 0;
		if(head->channels==2){
			if(channel==2)
				offset += head->bps/8;
		}
		if(head->bps==8){
			int32_t samp = *(uint8_t*)(data+offset)-128;
			if(head->channels==2 && channel==3){
				samp += *(uint8_t*)(data+offset+1)-128;
				samp>>=1;
			}
			val = ((double)samp)/128.0;
		}else if(head->bps==16){
			int32_t samp = *(int16_t*)(data+offset);
			if(head->channels==2 && channel==3){
				samp += *(int16_t*)(data+offset+2);
				samp>>=1;
			}
			val = ((double)samp)/32768.0;
		}
		(*out)[i]=val;
	}
}

void validate_header(wavhead* header){
	memcpy(header->chunkID,"RIFF",4);
	memcpy(header->fmt,"WAVE",4);
	memcpy(header->subID,"fmt ",4);
	memcpy(header->dataID,"data",4);
	header->subSize=16;
	header->subFmt=1;
	header->chunkSize = 36 + header->dataSize;
	header->block = header->channels * header->bps/8;
	header->byteRate = header->block * header->sampleRate;
}