#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "fft.h"

void writeNoise(wavhead* header, FILE* out, double length, double freq){
    int sampLen = header->sampleRate*length;
    uint8_t* data = malloc(sampLen*header->block);
    int16_t* shortdata = (int16_t*)data;
    double new=0, old=0;
    int period = header->sampleRate/freq;
    for(int i=0; i<sampLen; i++){
        if(i%period==0){
            old = new;
            new = (double)rand()/RAND_MAX/10;
        }
        double dif = new-old;
        double sum = new+old;
        if(dif<0)dif=0;
        if(dif>1)dif=1;
        if(sum<0)sum=0;
        if(sum>1)sum=1;
        if(header->bps==8){
            uint8_t sample = (uint8_t)(dif*255);
            for(int c=0; c<header->channels; c++){
                data[i*header->channels+c]=sample;
            }
        }else{
            int16_t sample = (int16_t)(dif*65535-32768);
            for(int c=0; c<header->channels; c++){
                shortdata[i*header->channels+c]=sample;
            }
        }
    }
    fwrite(header,sizeof(wavhead),1,out);
    fwrite(data,1,sampLen*header->block,out);
}

int main(){
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