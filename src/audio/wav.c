#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <string.h>
#include "../files/safeio.h"
#include "wav.h"

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