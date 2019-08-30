#ifndef _H_VOWEL
#define _H_VOWEL

#include "filter.h"

typedef struct _vowel{
	rfunc* zeros;
	rfunc* poles;
} vowel;

vowel* make_vowel(double,double*,double*,int);
void free_vowel(vowel*);
void make_vowels(double);
void clear_vowels();

extern vowel *V_A, *V_E, *V_I, *V_O, *V_U;

typedef struct _consonant{
	rfunc* zeros;
	rfunc* poles;
	double voice;
	double voiceless;
	double start[2];
	double release[2];
	double low[2];
	double stop[2];
	double attack[2];
	double decay[2];
	double sustain[2];
	double wait;
	double peak;
	double stay;
	double end;
} consonant;

extern consonant *C_N, *C_L;

typedef struct _phoneme{
	double amplitude;
	double frequency;
	double sweep;
	double length;
	wavegen func;
} phoneme;

void play_phoneme(vowel*, consonant*, phoneme*, double, double*, double*);

#endif