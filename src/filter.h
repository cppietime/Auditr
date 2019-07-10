#ifndef _H_FILTER
#define _H_FILTER

#include <complex.h>

typedef struct _rfunc{
	int order;
	double* coefs;
} rfunc;

void rfunc_free(rfunc*);
void rfunc_print(rfunc*);
rfunc* rfunc_mul(rfunc*, rfunc*);
rfunc* rfunc_conj(double,double);
rfunc* for_roots_r(double*,int);
complex double rfunc_calc(rfunc*,complex double);
rfunc* all_poles(double,double,double*,int);
rfunc* point_poles(double,double*,double*,int);
double rfunc_max_response(rfunc*,rfunc*,double,double*,int);
void rfunc_divhead(rfunc*);
void rfunc_normalize(rfunc*,double);
void filter(double*,double*,int,rfunc*,rfunc*);
void filter_in_place(double*,int,rfunc*,rfunc*);

typedef struct _vowel{
	rfunc* zeros;
	rfunc* poles;
} vowel;

vowel* make_vowel(double,double*,double*,int);
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

extern consonant *C_P;

typedef struct _phoneme{
	double amplitude;
	double frequency;
	double length;
} phoneme;

void play_phoneme(vowel*, consonant*, phoneme*, double, double*, double*);

#endif