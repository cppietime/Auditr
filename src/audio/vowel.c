#include <stdio.h>
#include <complex.h>
#include <stdlib.h>
#include "filter.h"
#include "vowel.h"

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

void free_vowel(vowel* v){
	if(v->zeros!=NULL)
		rfunc_free(v->zeros);
	if(v->poles!=NULL)
		rfunc_free(v->poles);
	free(v);
}

vowel *V_A, *V_E, *V_I, *V_O, *V_U;
consonant *C_N, *C_L, *C_M;
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
	double fT[] = {400,1600,2600,0,0};
	vowel* tmp = make_vowel(srate,fT,mags,3);
	C_N = malloc(sizeof(consonant));
	*C_N = (consonant){tmp->zeros,tmp->poles,1,0,{1,0},{0,.0},{0,0},{.06,0},{.06,.0},{.13,0},{1,0},0,.0,.12,.12};
	C_L = malloc(sizeof(consonant));
	*C_L = (consonant){tmp->zeros,tmp->poles,1,0,{0,0},{0,0},{0,0},{.02,.02},{.1,.01},{.14,.01},{.7,0},0,0,.12,.26};
	free(tmp);
	double fP[] = {400,1100,2150,0,0};
	tmp = make_vowel(srate,fP,mags,3);
	C_M = malloc(sizeof(consonant));
	*C_M = (consonant){tmp->zeros,tmp->poles,1,0,{1,0},{0,.0},{0,0},{.06,0},{.1,.0},{.13,0},{1,0},0,.0,.12,.12};
	free(tmp);
}

void clear_vowels(){
	rfunc_free(V_A->zeros);
	rfunc_free(V_E->zeros);
	rfunc_free(V_I->zeros);
	rfunc_free(V_O->zeros);
	rfunc_free(V_U->zeros);
	rfunc_free(C_N->zeros);
	rfunc_free(C_M->zeros);
	rfunc_free(V_A->poles);
	rfunc_free(V_E->poles);
	rfunc_free(V_I->poles);
	rfunc_free(V_O->poles);
	rfunc_free(V_U->poles);
	rfunc_free(C_N->poles);
	rfunc_free(C_M->poles);
	free(V_A);
	free(V_E);
	free(V_I);
	free(V_O);
	free(V_U);
	free(C_N);
	free(C_L);
	free(C_M);
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
	double phase = 0;
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
		buffer[i] = voice*vamp*sound->func(phase) + voiceless*vlamp/RAND_MAX*rand();
		phase += (sound->frequency+sound->sweep*dur/sound->length)/srate;
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