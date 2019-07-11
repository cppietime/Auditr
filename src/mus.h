#ifndef _H_MUS
#define _H_MUS

#include "filter.h"

double sawtooth(double);
double wavesin(double);
double halfsin(double);
double quartsin(double);
double rectsin(double);
double square(double);
double noise(double);

double logmap(int);

typedef struct _instrument{
	wavegen driver;
	vowel *v1,
		*v2;
	struct _instrument *phase_mod,
		*freq_mod,
		*amp_mod,
		*weight_mod;
	float offset,
		phase,
		weight,
		amplitude,
		frequency,
		slide,
		shift,
		vib_amp,
		vib_freq,
		trem_amp,
		trem_freq,
		attack,
		decay,
		sustain,
		release,
		duration,
		pm_ratio;
} instrument;

void render_to(instrument*,double,double*,double*);
void init_instrs();
void cleanup_instrs();
void prop_duration(instrument*);
void prop_freq(instrument*);
void reset_phase(instrument*);

int endianness();
void swap_endianness(void*,int);

float read_float(FILE*);
void write_float(FILE*,float);

void code_to_wave(FILE*,FILE*);

typedef double (*pseudorand)(int);

typedef struct _measure{
	int len;
	double notedata[0];
} measure;

typedef struct _track{
	int len;
	measure* measures[0];
} track;

measure* gen_measure(int,pseudorand);
track* gen_master_track(int,int,pseudorand);
track* gen_play_track(int,track*,pseudorand);
void write_track(FILE*,track*,pseudorand);
void init_file(FILE* out,double srate, double basefreq, double bps, double beats);

#endif