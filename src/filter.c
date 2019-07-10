#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "filter.h"
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

vowel* make_vowel(double srate, double* freqs, double* mags, int ord_poles){
	freqs[ord_poles+1]=srate/2;
	vowel* ret = malloc(sizeof(vowel));
	rfunc* poles = point_poles(srate,mags,freqs,ord_poles);
	double factor = rfunc_max_response(NULL,poles,srate,freqs,ord_poles+2);
	double* coefs = malloc(sizeof(double));
	*coefs = 1/factor;
	rfunc* zeros = malloc(sizeof(rfunc));
	zeros->order=0;
	zeros->coefs=coefs;
	*ret = (vowel){zeros,poles};
	return ret;
}

vowel *V_A, *V_E, *V_I, *V_O, *V_U;
consonant *C_P;
void make_vowels(double srate){
	double fA[] = {660,1700,2400,0,0};
	double fE[] = {270,2300,3000,0,0};
	double fI[] = {400,2000,2500,0,0};
	double fO[] = {300,870,2250,0,0};
	double fU[] = {640,1200,2400,0,0};
	double mags[] = {.99,.99,.99};
	V_A = make_vowel(srate,fA,mags,3);
	V_E = make_vowel(srate,fE,mags,3);
	V_I = make_vowel(srate,fI,mags,3);
	V_O = make_vowel(srate,fO,mags,3);
	V_U = make_vowel(srate,fU,mags,3);
	double fP[] = {400,1600,2600,0,0};
	vowel* tmp = make_vowel(srate,fP,mags,3);
	C_P = malloc(sizeof(consonant));
	*C_P = (consonant){tmp->zeros,tmp->poles,.5,1,{1,.1},{0,.04},{0,0},{.04,.1},{.0,.0},{.13,.024},{1,0},0,.025,.14,.02};
	free(tmp);
}

void clear_vowels(){
	free(V_A->zeros);
	free(V_E->zeros);
	free(V_I->zeros);
	free(V_O->zeros);
	free(V_U->zeros);
	free(V_A->poles);
	free(V_E->poles);
	free(V_I->poles);
	free(V_O->poles);
	free(V_U->poles);
	free(V_A);
	free(V_E);
	free(V_I);
	free(V_O);
	free(V_U);
	free(C_P);
}

void play_phoneme(vowel* formant, consonant* stop, phoneme* sound, double srate, double* dst, double* end){
	double* buffer = calloc(sound->length*srate, sizeof(double));
	double voice = sound->amplitude;
	double voiceless = 0;
	if(stop!=NULL){
		voice *= stop->voice;
		voiceless = sound->amplitude * stop->voiceless;
	}
	int period = srate/sound->frequency;
	double vamp = 1;
	double vlamp = 1;
	int i;
	for(i=0; i<(int)(sound->length*srate); i++){
		double dur = i/srate;
		if(stop!=NULL){
			if(dur<stop->release[0]&&stop->release[0]>0)
				vamp -= (stop->start[0]-stop->low[0])/(stop->release[0]*srate);
			else if(dur<stop->stop[0]+stop->release[0]&&stop->stop[0]>0)
				vamp = stop->low[0];
			else if(dur<stop->attack[0]+stop->stop[0]+stop->release[0]&&stop->attack[0]>0)
				vamp += 1.0/(stop->attack[0]*srate);
			else if(dur<stop->decay[0]+stop->attack[0]+stop->stop[0]+stop->release[0]&&stop->decay[0]>0)
				vamp -= (1-stop->sustain[0])/(stop->decay[0]*srate);
			else
				vamp = stop->sustain[0];
			if(dur<stop->release[1]&&stop->release[1]>0)
				vlamp -= (stop->start[1]-stop->low[1])/stop->release[1]/srate;
			else if(dur<stop->stop[1]+stop->release[1]&&stop->stop[1]>0)
				vlamp = stop->low[1];
			else if(dur<stop->attack[1]+stop->stop[1]+stop->release[1]&&stop->attack[1]>0)
				vlamp += 1.0/stop->attack[1]/srate;
			else if(dur<stop->decay[1]+stop->attack[1]+stop->stop[1]+stop->release[1]&&stop->decay[1]>0)
				vlamp = 1;//-= (1-stop->sustain[1])/stop->decay[1]/srate;
			else
				vlamp = stop->sustain[1];
		}
		buffer[i] = voice*vamp/period*(i%period) + voiceless*vlamp/RAND_MAX*rand();
	}
	double* cbuf = NULL;
	if(stop!=NULL){
		cbuf = calloc(sound->length*srate,sizeof(double));
		filter(cbuf,buffer,sound->length*srate,stop->zeros,stop->poles);
	}
	if(formant!=NULL)
		filter_in_place(buffer,sound->length*srate,formant->zeros,formant->poles);
	double factor = 0;
	for(i=0; i<(int)(sound->length*srate) && (dst+i)<end; i++){
		double point = buffer[i];
		if(cbuf!=NULL){
			if(i<stop->wait*srate)
				factor = 0;
			if(i<(stop->peak+stop->wait)*srate)
				factor += 1.0/(stop->peak*srate);
			else if(i<(stop->stay+stop->peak+stop->wait)*srate)
				factor = 1;
			else if(i<(stop->end+stop->stay+stop->peak+stop->wait)*srate)
				factor -= 1.0/(stop->end*srate);
			else
				factor = 0;
			point *= (1-factor);
			point += cbuf[i] * factor;
		}
		dst[i] += point;
	}
	free(buffer);
	if(cbuf!=NULL)
		free(cbuf);
}

int main(){
	int srate = 44100;
	make_vowels(srate);
	int secs = 2;
	int length = secs*srate;
	double* samps = malloc(sizeof(double)*length);
	phoneme phone = {1,100,2./3};
	play_phoneme(V_A,C_P,&phone,(srate),samps,samps+length);
	phone.frequency=126;
	play_phoneme(V_U,C_P,&phone,(srate),samps+length/3,samps+length);
	phone.frequency=133;
	play_phoneme(V_U,C_P,&phone,(srate),samps+length/3*2,samps+length);
	wavhead header;
	header.channels=1;
	header.sampleRate=srate;
	header.bps=16;
	header.dataSize = 2*length;
	validate_header(&header);
	int16_t* data = malloc(sizeof(int16_t)*length);
	int max = 0;
	int i;
	for(i=0; i<length; i++){
		data[i] = samps[i]*32767;
		if(data[i]>max)max=data[i];
	}
	printf("Max: %d\n",max);
	FILE* output = fopen("train.wav","wb");
	fwrite(&header,sizeof(header),1,output);
	fwrite(data,2,length,output);
	fclose(output);
	free(samps);
	free(data);
	clear_vowels();
	return 0;
}