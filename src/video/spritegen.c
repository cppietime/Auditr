#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <sys/time.h>
#include "bmp.h"
#include "spritegen.h"

float perlin_bias(float t, float x){
    return t/((((1.0/x)-2.0)*(1.0-t))+1.0);
}

float perlin_gain(float t, float x){
    if(t<=.5){
        return perlin_bias(t*2.0, x)/2.0;
    }else{
        return perlin_bias(t*2.0-1.0, 1.0 - x)/2.0 + .5;
    }
}

uint8_t get_pixel(Bitmap* canvas, int x, int y){
    uint8_t bitmask = 0xff<<(8-canvas->bps);
    uint8_t mask = bitmask >> (canvas->bps * (x%(8/canvas->bps)));
    int index = y*canvas->row_bytes + (x*canvas->bps)/8;
    uint8_t* pixdata = (uint8_t*)canvas->bitmap;
    uint16_t ret = pixdata[index] & mask;
    ret >>= canvas->bps * ((8/canvas->bps - x)%(8/canvas->bps));
    return ret&0xff;
}

void set_pixel(Bitmap* canvas, int x, int y, uint16_t pixel){
    uint8_t bitmask = 0xff<<(8-canvas->bps);
    uint8_t mask = bitmask >> (canvas->bps * (x%(8/canvas->bps)));
    int index = y*canvas->row_bytes + (x*canvas->bps)/8;
    uint8_t* pixdata = (uint8_t*)canvas->bitmap;
    pixdata[index] &= ~mask;
    pixel <<= 8-canvas->bps;
    pixel >>= canvas->bps * (x%(8/canvas->bps));
    pixdata[index] |= pixel&0xff;
}

void put_pixel(Bitmap* canvas, Spr_params* params, int x, int y){
    int colors = canvas->colors;
    if(colors==0)colors = 1<<canvas->bps;
    float dist = hypot(x-canvas->width/2, y-canvas->height/2)/hypot(canvas->width/2,canvas->height/2);
    float prob = ((float)rand())/RAND_MAX;
    prob = params->min + perlin_bias(perlin_gain(prob, params->gain), params->bias)/(params->max-params->min);
    uint16_t pix = 0;
    if(prob>=dist){
        pix = 1+(rand()%(colors-1));
        set_pixel(canvas, x, y, pix);
    }
}

void copy_pixel(Bitmap* canvas, int dx, int dy, int sx, int sy){
    uint8_t src = get_pixel(canvas, sx, sy);
    set_pixel(canvas, dx, dy, src);
}

int count_surrounding(Bitmap* canvas, int x, int y){
    int ret = 0;
    if(x>0){
        ret += get_pixel(canvas, x-1, y)!=0;
    }
    if(x+1<canvas->width){
        ret += get_pixel(canvas, x+1, y)!=0;
    }
    if(y>0){
        ret += get_pixel(canvas, x, y-1)!=0;
    }
    if(y+1<canvas->height){
        ret += get_pixel(canvas, x, y+1)!=0;
    }
    return ret;
}

uint8_t random_neighbor(Bitmap* canvas, int x, int y){
    int dir = rand()&3;
    switch(dir){
        case 0:
            return get_pixel(canvas, x-1, y);
        case 1:
            return get_pixel(canvas, x+1, y);
        case 2:
            return get_pixel(canvas, x, y-1);
        case 3:
            return get_pixel(canvas, x, y+1);
    }
}

void smooth(Bitmap* canvas, float prob){
    for(int x=0; x<canvas->width; x++){
        for(int y=0; y<canvas->height; y++){
            if(get_pixel(canvas, x, y)){ //should we remove?
                if(!count_surrounding(canvas, x, y) && ((float)rand())/RAND_MAX <= prob){
                    set_pixel(canvas, x, y, 0);
                }
            }else{ //should we add?
                if(count_surrounding(canvas, x, y)==4 && ((float)rand())/RAND_MAX <= prob){
                    set_pixel(canvas, x, y, random_neighbor(canvas, x, y));
                }
            }
        }
    }
}

void make_sprite(Bitmap* canvas, unsigned int seed, Spr_params* params){
    srand(seed);
    int mirX = params->x_prob >= ((float)rand())/RAND_MAX;
    int mirY = params->y_prob >= ((float)rand())/RAND_MAX;
    int maxX = mirY?canvas->width/2:canvas->width;
    int maxY = mirX?canvas->height/2:canvas->height;
    for(int x=0; x<maxX; x++){
        for(int y=0; y<maxY; y++){
            put_pixel(canvas, params, x, y);
        }
    }
    smooth(canvas, params->sm_prob);
    if(mirY){
        for(int x=maxX; x<canvas->width; x++){
            for(int y=0; y<canvas->height; y++){
                copy_pixel(canvas, x, y, 2*maxX-x-1, y);
            }
        }
    }
    if(mirX){
        for(int y=maxY; y<canvas->height; y++){
            for(int x=0; x<canvas->width; x++){
                copy_pixel(canvas, x, y, x, 2*maxY-y-1);
            }
        }
    }
    if(params->p_prob >= ((float)rand())/RAND_MAX){
        for(int y=0; y<canvas->height; y++){
            for(int x=y; x<canvas->width; x++){
                if(x < canvas->height && y < canvas->width){
                    copy_pixel(canvas, x, y, y, x);
                }
            }
        }
    }
    if(params->n_prob >= ((float)rand())/RAND_MAX){
        for(int y=0; y<canvas->height; y++){
            for(int x=canvas->width-y; x<canvas->width; x++){
                if(canvas->width-x < canvas->height && canvas->height-y < canvas->width){
                    copy_pixel(canvas, x, y, canvas->height-y, canvas->width-x);
                }
            }
        }
    }
}

int main(){
    unsigned int seed = time(NULL);
    srand(seed);
    int w = 64,h = 64;
    Bitmap* bmp = Bmp_empty(w, h, 8, 8);
    memset(bmp->bitmap, 0, w*h);
    bmp->palette[0] = 0;
    bmp->R = bmp->G = bmp->B = 0;
    for(int i=1; i<8; i++){
        bmp->palette[i] = (rand() + (rand()<<15))&0xffffff;
    }
    Spr_params params = {.25, .85, 0, .999, .9, .5, .5, .5, .5, MIRROR_NEG | MIRROR_X | MIRROR_Y};
    make_sprite(bmp, seed, &params);
    FILE* out = fopen("test.bmp", "wb");
    Bmp_save(bmp, out);
    fclose(out);
    Bmp_free(bmp);
    return 0;
}