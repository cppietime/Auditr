#ifndef _H_FILTER
#define _H_FILTER

#include <complex.h>

//Polynomial with real coefficients
typedef struct _rfunc{
	int order;
	double* coefs;
} rfunc;

void rfunc_free(rfunc*);
void rfunc_print(rfunc*);
rfunc* rfunc_mul(rfunc*, rfunc*);
//Construct rfunc from two conjugate roots
rfunc* rfunc_conj(double,double);
//Contruct rfunc with conjugate roots
rfunc* for_roots_r(double*,int);
complex double rfunc_calc(rfunc*,complex double);
//rfunc for poles at frequencies given
rfunc* all_poles(double,double,double*,int);
//same as all_poles but with independent magnitudes
rfunc* point_poles(double,double*,double*,int);
//find magnitude of highest response for rfunc
double rfunc_max_response(rfunc*,rfunc*,double,double*,int);
//normalize rfunc by first coefficient
void rfunc_divhead(rfunc*);
//nomralize by given factor
void rfunc_normalize(rfunc*,double);
//apply filter from one array to another
void filter(double*,double*,int,rfunc*,rfunc*);
//apply filter in place
void filter_in_place(double*,int,rfunc*,rfunc*);

typedef double (*wavegen)(double);

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