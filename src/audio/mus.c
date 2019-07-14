#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include "fft.h"
#include "filter.h"
#include "mus.h"

#define PLAYERS 16
#define VOWELS 16

instrument players[PLAYERS];
vowel* vowels[VOWELS];

double sawtooth(double arg){
	return fabs(arg-(int)arg)*2-1;
}

double wavesin(double arg){
	return sin(arg*2*M_PI);
}

double halfsin(double arg){
	arg = fabs(arg-(int)arg);
	return (arg<.5)?sin(arg*2*M_PI):0;
}

double quartsin(double arg){
	arg = fabs(arg-(int)arg);
	return (arg<.25)?sin(arg*2*M_PI):0;
}

double rectsin(double arg){
	return fabs(sin(arg*2*M_PI));
}

double square(double arg){
	return (fabs(arg-(int)arg)<.5)?1.0:-1.0;
}

double noise(double arg){
	return (double)rand()/RAND_MAX*2.0-1.0;
}

static double logmap_x = .1, logmap_r = 3.8;

double logmap(int max){
	logmap_x *= (1-logmap_x)*logmap_r;
	return fmod(logmap_x*max,max);
}

double truerand(int max){
	return (double)rand()/RAND_MAX*max;
}

#define RANDOMS 2
pseudorand randoms[] = {logmap, truerand};

#define DRIVERS 21
wavegen drivers[] = {
	wavesin, wavesin, //2
	halfsin, halfsin, halfsin, //3
	quartsin, quartsin, quartsin, quartsin, //4
	rectsin, rectsin, //2
	sawtooth, sawtooth, sawtooth, sawtooth, sawtooth, sawtooth, sawtooth, //7
	square, square, //2
	noise //1
	};

void render_to(instrument* sound, double srate, double* dst, double* end){
	// printf("Note freq:%lf amp:%lf dur:%lf\n",sound->frequency,sound->amplitude,sound->duration);
	int size = (sound->duration+sound->release)*srate;
	double *phase_buf = NULL,
		*amp_buf = NULL,
		*freq_buf = NULL,
		*weight_buf = NULL;
	if(sound->phase_mod!=NULL){
		phase_buf = calloc(size,sizeof(double));
		render_to(sound->phase_mod,srate,phase_buf,phase_buf+size);
	}
	if(sound->freq_mod!=NULL){
		freq_buf = calloc(size,sizeof(double));
		render_to(sound->freq_mod,srate,freq_buf,freq_buf+size);
	}
	double* buffer = calloc(size,sizeof(double));
	double freq_adj = pow(2,sound->slide*1.0/size);
	int i;
	for(i=0; i<size; i++){
		double phase = sound->phase;
		if(phase_buf!=NULL)
			phase += phase_buf[i];
		buffer[i] = sound->offset;
		if(sound->driver!=NULL)
			buffer[i] += sound->driver(phase);
		double freq = sound->frequency;
		double fpow = sin(i*2*M_PI*sound->vib_freq/srate)*sound->vib_amp;
		if(freq_buf!=NULL)
			fpow += freq_buf[i];
		freq *= pow(2,fpow/12.0);
		sound->frequency *= freq_adj;
		sound->phase += freq/srate;
	}
	if(phase_buf!=NULL){
		free(phase_buf);
		phase_buf=NULL;
	}
	if(freq_buf!=NULL){
		free(freq_buf);
		freq_buf=NULL;
	}
	if(sound->amp_mod!=NULL){
		amp_buf = calloc(size,sizeof(double));
		render_to(sound->amp_mod,srate,amp_buf,amp_buf+size);
	}
	double envelope = 0;
	double time = 0;
	for(i=0; i<size; i++){
		if(time<sound->attack)
			envelope += 1.0/sound->attack/srate;
		else if(time<sound->attack+sound->decay)
			envelope -= (1.0-sound->sustain)/sound->decay/srate;
		else if(time<sound->duration)
			envelope = 1;
		else
			envelope -= sound->sustain/sound->release/srate;
		double amplitude = sound->amplitude * envelope;
		amplitude += sin(i*2*M_PI*sound->trem_freq/srate)*sound->trem_amp;
		if(amp_buf!=NULL)
			amplitude += amp_buf[i];
		buffer[i] *= amplitude;
		time += 1.0/srate;
	}
	if(amp_buf!=NULL){
		free(amp_buf);
		amp_buf=NULL;
	}
	if(sound->v1!=NULL){
		double* filt1 = calloc(size,sizeof(double));
		filter(filt1,buffer,size,sound->v1->zeros,sound->v1->poles);
		if(sound->v2!=NULL){
			double* filt2 = calloc(size,sizeof(double));
			filter(filt2,buffer,size,sound->v2->zeros,sound->v2->poles);
			if(sound->weight_mod!=NULL){
				weight_buf = calloc(size,sizeof(double));
				render_to(sound->weight_mod,srate,weight_buf,weight_buf+size);
			}
			for(i=0; i<size; i++){
				double weight = sound->weight;
				if(weight_buf!=NULL)
					weight += weight_buf[i];
				if(weight<0)weight=0;
				else if(weight>1)weight=1;
				buffer[i] = filt2[i] * weight + filt1[i] * (1-weight);
				sound->weight += sound->shift/size;
			}
			free(filt1);
			free(filt2);
			if(weight_buf!=NULL){
				free(weight_buf);
				weight_buf=NULL;
			}
		}else{
			free(buffer);
			buffer = filt1;
			filt1 = NULL;
		}
	}
	if(size>end-dst)
		size=end-dst;
	for(i=0; i<size; i++){
		dst[i]+=buffer[i];
	}
	free(buffer);
}

void init_instrs(){
	int i;
	for(i=0; i<PLAYERS; i++){
		instrument* inst = players+i;
		*inst = (instrument){
			.driver = wavesin,
			.v1 = NULL,
			.v2 = NULL,
			.phase_mod = NULL,
			.amp_mod = NULL,
			.freq_mod = NULL,
			.weight_mod = NULL,
			.offset = 0,
			.phase = 0,
			.weight = 0,
			.amplitude = 1,
			.frequency = 55,
			.slide = 0,
			.shift = 0,
			.vib_amp = 0,
			.vib_freq = 0,
			.trem_amp = 0,
			.trem_freq = 0,
			.attack = 0,
			.decay = 0,
			.sustain = 1,
			.release = 0,
			.duration = 0,
			.pm_ratio = 0
		};
	}
	for(i=0; i<VOWELS; i++){
		vowels[i] = NULL;
	}
}

void cleanup_instrs(){
	int i;
	for(i=0; i<VOWELS; i++){
		if(vowels[i]!=NULL)
			free_vowel(vowels[i]);
	}
}

void prop_duration(instrument* inst){
	if(inst->phase_mod!=NULL){
		inst->phase_mod->duration=inst->duration;
		prop_duration(inst->phase_mod);
	}
	if(inst->freq_mod!=NULL){
		inst->freq_mod->duration=inst->duration;
		prop_duration(inst->freq_mod);
	}
	if(inst->amp_mod!=NULL){
		inst->amp_mod->duration=inst->duration;
		prop_duration(inst->amp_mod);
	}
	if(inst->weight_mod!=NULL){
		inst->weight_mod->duration=inst->duration;
		prop_duration(inst->weight_mod);
	}
}

//Propagates frequency change through phase modulator, if it exists
void prop_freq(instrument* inst){
	if(inst->phase_mod!=NULL){
		inst->phase_mod->frequency=inst->frequency*inst->pm_ratio;
		prop_freq(inst->phase_mod);
	}
}

//Reset self and all children's phases
void reset_phase(instrument* inst){
	inst->phase=0;
	if(inst->phase_mod!=NULL){
		reset_phase(inst->phase_mod);
	}
	if(inst->freq_mod!=NULL){
		reset_phase(inst->freq_mod);
	}
	if(inst->amp_mod!=NULL){
		reset_phase(inst->amp_mod);
	}
	if(inst->weight_mod!=NULL){
		reset_phase(inst->weight_mod);
	}
}

//Tests whether bit or little endian
int endianness(){
	int a = 1;
	char b = *((char*)&a);
	return b==0;
}

static int big_endian = 2;

//When necessary, call this to swap memory's endianness
void swap_endianness(void* mem, int len){
	char* bytes = (char*)mem;
	int i;
	for(i=0; i<len>>1; i++){
		char tmp = bytes[i];
		bytes[i] = bytes[len-1-i];
		bytes[len-1-i] = tmp;
	}
}

//Read little-endian float
float read_float(FILE* stream){
	if(big_endian==2)
		big_endian = endianness();
	float ret;
	fread(&ret,sizeof(float),1,stream);
	if(big_endian)
		swap_endianness(&ret,sizeof(float));
	return ret;
}

//Write a float to a file little-endian
void write_float(FILE* stream, float val){
	if(big_endian==2)
		big_endian = endianness();
	if(big_endian)
		swap_endianness(&val,sizeof(float));
	fwrite(&val,sizeof(float),1,stream);
}

//Shidi interpreter
void code_to_wave(FILE* in, FILE* out){
	double* buffer = NULL;
	double srate = 44100;
	double bps = 4;
	double beats = 4;
	double base = 55;
	double accum = 0;
	int size = srate*beats/bps;
	int time = 0;
	char bits = 16;
	char op;
	int i;
	printf("Reading file\n");
	while(!feof(in)){
		op = fgetc(in);
		if(op==EOF || feof(in))
			break;
		switch(op){
			case 33: //Set beats/second
				bps = read_float(in);
				break;
			case 32: //Set duration in beats
				beats = read_float(in);
				break;
			case 35: //Set samps/second
				srate = read_float(in);
				break;
			case 37: //Generate buffer
				size = srate*beats/bps;
				if(buffer!=NULL)
					free(buffer);
				buffer = calloc(size,sizeof(double));
				break;
			case 34: //Set base frequency
				base = read_float(in);
				break;
			case 36: //Set bitdepth
				bits = fgetc(in);
				break;
			case 38:{ //Jump to time in beats
				double tobeats = read_float(in);
				time = srate*tobeats/bps;
			}break;
			case 31:{ //Construct vowel
				int index = fgetc(in);
				double pfreqs[10], pmags[8];
				int ps=0;
				for(i=0; i<8; i++){
					double val = read_float(in);
					if(val!=val)
						break;
					pfreqs[ps++]=val;
				}
				pfreqs[ps] = 0;
				pfreqs[ps+1] = srate/2;
				for(i=0; i<ps; i++){
					pmags[i] = read_float(in);
				}
				if(vowels[index]!=NULL)
					free_vowel(vowels[index]);
				vowels[index] = make_vowel(srate,pfreqs,pmags,ps);
			}break;
			case 28:{ //Progress time in beats
				double tobeats = read_float(in);
				time += tobeats*srate/bps;
			}break;
			case 29:{ //Play and reset phase
				char index = fgetc(in);
				render_to(players+index,srate,buffer+time,buffer+size);
				reset_phase(players+index);
			}break;
			case 30:{ //Play w/o resetting phase
				char index = fgetc(in);
				render_to(players+index,srate,buffer+time,buffer+size);
			}break;
			case 27:{ //Set accum to note frequency
				double note = read_float(in);
				accum = base * pow(2,note/12.);
			}break;
			case 1:{ //Set driver
				char index = fgetc(in);
				char drive = fgetc(in);
				players[index].driver = drivers[drive];
			}break;
			case 2:{ //Set vowel
				char index = fgetc(in);
				char vsel = (index&0x80)!=0;
				index &= 0x7f;
				char drive = fgetc(in);
				vowel* vowel = NULL;
				if(drive==0x7f)
					vowel=NULL;
				else if(drive<VOWELS)
					vowel=vowels[drive];
				else switch(drive-VOWELS){
					case 0:
						vowel=V_A;break;
					case 1:
						vowel=V_E;break;
					case 2:
						vowel=V_I;break;
					case 3:
						vowel=V_O;break;
					case 4:
						vowel=V_U;break;
				}
				if(vsel)
					players[index].v2=vowel;
				else
					players[index].v1=vowel;
			}break;
			case 3:{ //Set modulator
				char index = fgetc(in);
				unsigned char sel = index>>6;
				index &= 0x3f;
				char ptr = fgetc(in);
				instrument* target = NULL;
				if(ptr<PLAYERS)
					target = players+ptr;
				switch(sel){
					case 0:
						players[index].phase_mod=target;break;
					case 1:
						players[index].freq_mod=target;break;
					case 2:
						players[index].amp_mod=target;break;
					case 3:
						players[index].weight_mod=target;break;
				}
			}break;
			case 4:{ //Set DC
				char index = fgetc(in);
				players[index].offset = read_float(in);
			}break;
			case 5:{ //Set phase
				char index = fgetc(in);
				players[index].phase = read_float(in);
			}break;
			case 6:{ //Set weight
				char index = fgetc(in);
				players[index].weight = read_float(in);
			}break;
			case 7:{ //Set amplitude
				char index = fgetc(in);
				players[index].amplitude = read_float(in);
			}break;
			case 8:{ //Set frequency manually
				char index = fgetc(in);
				players[index].frequency = read_float(in);
			}break;
			case 9:{ //Set frequency to note val
				char index = fgetc(in);
				double note = read_float(in);
				players[index].frequency = base*pow(2,note/12.);
			}break;
			case 10:{ //Set frequency slide
				char index = fgetc(in);
				players[index].slide = read_float(in);
			}break;
			case 11:{ //Set weight shift
				char index = fgetc(in);
				players[index].shift = read_float(in);
			}break;
			case 12:{ //Set vibrato amplitude
				char index = fgetc(in);
				players[index].vib_amp = read_float(in);
			}break;
			case 13:{ //Set vibrato frequency
				char index = fgetc(in);
				players[index].vib_freq = read_float(in);
			}break;
			case 14:{ //Set tremelo amplitude
				char index = fgetc(in);
				players[index].trem_amp = read_float(in);
			}break;
			case 15:{ //Set tremelo frequency
				char index = fgetc(in);
				players[index].trem_freq = read_float(in);
			}break;
			case 16:{ //Set attack
				char index = fgetc(in);
				players[index].attack = read_float(in);
			}break;
			case 17:{ //Set decay
				char index = fgetc(in);
				players[index].decay = read_float(in);
			}break;
			case 18:{ //Set sustain
				char index = fgetc(in);
				players[index].sustain = read_float(in);
			}break;
			case 19:{ //Set release
				char index = fgetc(in);
				players[index].release = read_float(in);
			}break;
			case 20:{ //Set duration in beats, no propagation
				char index = fgetc(in);
				players[index].duration = read_float(in)/bps;
			}break;
			case 21:{ //Set phase mod ratio
				char index = fgetc(in);
				players[index].pm_ratio = read_float(in);
			}break;
			case 22:{ //Set duration in beats and propagate
				char index = fgetc(in);
				players[index].duration = read_float(in)/bps;
				prop_duration(players+index);
			}break;
			case 23:{ //Set frequency to note val and propagate
				char index = fgetc(in);
				players[index].frequency = base*pow(2,read_float(in)/12.);
				prop_freq(players+index);
			}break;
		}
	}
	wavhead header = {.bps=bits, .sampleRate=srate, .channels=1, .dataSize=bits/8*size};
	validate_header(&header);
	fwrite(&header,sizeof(wavhead),1,out);
	printf("Wrote RIFF Header\n");
	if(buffer!=NULL){
		printf("Have buffer\n");
		double max = 0;
		for(i=0; i<size; i++){
			if(fabs(buffer[i])>max)
				max=fabs(buffer[i]);
		}
		if(max<1)
			max=1;
		for(i=0; i<size; i++){
			buffer[i] /= max;
			if(bits==8){
				uint8_t samp = (buffer[i]/2+.5)*255;
				fputc(samp,out);
			}else{
				int16_t samp = buffer[i]*32767;
				fwrite(&samp,2,1,out);
			}
		}
		printf("Wrote buffer\n");
		free(buffer);
	}
}

double scale[24] = {0,	2,	4,	5,	7,	9,	11,
					12,	14,	16,	17,	19,	21,	23,
					24,	26,	28,	29,	31,	33,	35,
					36,	38,	40};
int microtonal = 0;

measure* gen_measure(int len, pseudorand func){
	measure* ret = malloc(sizeof(measure)+sizeof(double)*len*3);
	ret->len=len;
	int i;
	double runtime = 0;
	for(i=0; i<len; i++){
		int index = (int)func(24);
		if(microtonal==0)
			ret->notedata[i*3]=scale[index];
		else if(microtonal==1)
			ret->notedata[i*3]=index;
		else
			ret->notedata[i*3]=index+1.0*(int)func(microtonal)/microtonal;
	}
	for(i=0; i<len; i++){
		ret->notedata[i*3+1]=func(len/2)+1;
		if(runtime+ret->notedata[i*3+1]>len)
			ret->notedata[i*3+1]=len-runtime;
		ret->notedata[i*3+2]=func(len/2)+1;
		if(runtime+ret->notedata[i*3+2]>len)
			ret->notedata[i*3+2]=len-runtime;
		if(i+1<len)
			runtime += ret->notedata[i*3+2];
	}
	ret->notedata[len*3-1]=len-runtime;
	return ret;
}

//Master tracks are not played, they are essentially lists of available measures
track* gen_master_track(int len, int measure_len, pseudorand func){
	track* master = malloc(sizeof(track)+sizeof(measure*)*len);
	master->len=len;
	int i;
	for(i=0; i<len; i++){
		master->measures[i] = gen_measure(measure_len,func);
	}
	return master;
}

void free_master_track(track* track){
	int i;
	for(i=0; i<track->len; i++)
		free(track->measures[i]);
	free(track);
}

//These can be freed just with free
track* gen_play_track(int len, track* master, pseudorand func){
	track* ret = malloc(sizeof(track)+sizeof(measure*)*len);
	ret->len=len;
	int i;
	for(i=0; i<len; i++){
		ret->measures[i] = master->measures[(int)(func(master->len))];
	}
	return ret;
}

//Appends track to file, starting at time=0. init_file first
void write_track(FILE* out, track* track, pseudorand func){
	//Set v1
	fputc(2,out);
	fputc(0,out);
	fputc(VOWELS+func(5),out);
	if((int)func(2)){
		//Set v2
		fputc(2,out);
		fputc(0|0x80,out);
		fputc(VOWELS+func(5),out);
		//0->weight
		fputc(6,out);
		fputc(0,out);
		write_float(out,0);
		//1->shift
		fputc(11,out);
		fputc(0,out);
		write_float(out,1);
	}
	//Set driver
	char drive = (char)func(DRIVERS);
	fputc(1,out);
	fputc(0,out);
	fputc(drive,out);
	if(drivers[drive]!=square&&drivers[drive]!=sawtooth&&(int)func(2)){
		//Set phase modulation to instrument 1
		fputc(3,out);
		fputc(0,out);
		fputc(1,out);
		//Choose freq and ratio
		int ratio = 1<<((int)func(3)+1);
		double ampl = 2./ratio;
		//Set inst 1 to sine
		fputc(1,out);
		fputc(1,out);
		fputc(1,out);
		//Set inst ampl
		fputc(7,out);
		fputc(1,out);
		write_float(out,ampl);
		//Set PM ratio
		fputc(21,out);
		fputc(0,out);
		write_float(out,ratio);
	}else{
		//Ensure no PM
		fputc(3,out);
		fputc(0,out);
		fputc(PLAYERS+1,out);
	}
	//Set ADSR
	fputc(16,out);
	fputc(0,out);
	write_float(out,func(1)*.2);
	fputc(17,out);
	fputc(0,out);
	write_float(out,func(1)*.2);
	fputc(18,out);
	fputc(0,out);
	write_float(out,1-func(1)*.6);
	fputc(19,out);
	fputc(0,out);
	write_float(out,func(1)*.1);
	//Jump to begin
	fputc(38,out);
	write_float(out,0);
	int i,j;
	for(i=0; i<track->len; i++){
		measure* meas = track->measures[i];
		for(j=0; j<meas->len; j++){
			if(meas->notedata[j*3+1]>0){
				fputc(23,out);
				fputc(0,out);
				write_float(out,meas->notedata[j*3]);
				fputc(22,out);
				fputc(0,out);
				write_float(out,meas->notedata[j*3+1]);
				fputc(29,out);
				fputc(0,out);
				fputc(28,out);
			}
			write_float(out,meas->notedata[j*3+2]);
		}
	}
}

//Call once per file to set up
void init_file(FILE* out,double srate, double basefreq, double bps, double beats, int tracks){
	//srate
	fputc(35,out);
	write_float(out,srate);
	//basefreq
	fputc(34,out);
	write_float(out,basefreq);
	//bps
	fputc(33,out);
	write_float(out,bps);
	//duration
	fputc(32,out);
	write_float(out,beats);
	//buffer
	fputc(37,out);
	//amplitude
	fputc(7,out);
	fputc(0,out);
	write_float(out,1.0/tracks);
}

int main(int argc, char** argv){
	char name[128] = "shidi.wav";
	if(argc>1)
		strcpy(name,argv[1]);
	srand(time(NULL));
	pseudorand choice = randoms[rand()%RANDOMS];
	logmap_r = rand()*0.4/RAND_MAX+3.6;
	double srate = 44100,
		bps = choice(6)+1,
		bpm = 2*((int)choice(2)+1),
		mpb = 4*((int)choice(15)+2)+1,
		uni_meas = (int)choice(9)+3,
		basefreq = 55 * pow(2,choice(12)/12.0);
	int tracknum = (int)choice(3)+1;
	make_vowels(srate);
	init_instrs();
	FILE* shid = fopen("shidi.shid","wb");
	microtonal = (int)choice(4);
	init_file(shid,srate,basefreq,bps,bpm*mpb,tracknum);
	int i;
	for(i=0; i<tracknum; i++){
		track* master = gen_master_track(uni_meas, bpm, choice);
		track* play_track = gen_play_track(mpb, master, choice);
		write_track(shid, play_track, choice);
		free_master_track(master);
		free(play_track);
	}
	fclose(shid);
	shid = fopen("shidi.shid","rb");
	FILE* wave = fopen(name,"wb");
	code_to_wave(shid,wave);
	fclose(shid);
	fclose(wave);
	cleanup_instrs();
	clear_vowels();
	return 0;
}