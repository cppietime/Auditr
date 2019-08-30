#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include "fft.h"
#include "filter.h"

void writeNoise(wavhead* header, FILE* out, double length, double freq){
    int sampLen = header->sampleRate*length;
    double* samples = malloc(sizeof(double)*sampLen);
    uint8_t* data = malloc(sampLen*header->block);
    int16_t* shortdata = (int16_t*)data;
    double new=0, old=0;
    int period = header->sampleRate/freq;
    for(int i=0; i<sampLen; i++){
        new = (double)rand()/RAND_MAX/1;
        double dif = new;
        double sum = new;
        if(dif<0)dif=0;
        if(dif>1)dif=1;
        if(sum<0)sum=0;
        if(sum>1)sum=1;
        samples[i] = dif;
    }
    double mag = .8;
    double pfreq[3] = {freq, 0, 0};
    vowel* filter = make_vowel(header->sampleRate, pfreq, &mag, 1);
    filter_in_place(samples, sampLen, filter->zeros, filter->poles);
    printf("Filtered in place\n");
    for(int i=0; i<sampLen; i++){
        if(samples[i]>1)samples[i]=1;
        if(samples[i]<0)samples[i]=0;
        if(header->bps==8){
            uint8_t samp = (samples[i]*255);
            for(int c=0; c<header->channels; c++){
                data[i*header->channels+c] = samp;
            }
        }else{
            int16_t samp = (samples[i]-.5)*2*32767;
            for(int c=0; c<header->channels; c++){
                shortdata[i*header->channels+c] = samp;
            }
        }
    }
    fwrite(header,sizeof(wavhead),1,out);
    fwrite(data,1,sampLen*header->block,out);
}

int main(){
    srand(time(NULL));
    wavhead header = {.sampleRate=44100, .channels=1, .bps=8};
    validate_header(&header);
    header.dataSize = header.block*header.sampleRate*5;
    char filename[16];
    for(int i=0; i<10; i++){
        int freq = 44100/20*(i+1);
        sprintf(filename,"noise-%d.wav",freq);
        FILE* wav = fopen(filename, "wb");
        writeNoise(&header, wav, 5, freq);
        fclose(wav);
    }
    return 0;
}