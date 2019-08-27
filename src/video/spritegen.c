#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/time.h>
#include "bmp.h"
#include "spritegen.h"

static uint32_t seed = 0;
static uint32_t rand_max_replace = 1<<30;

void srand_replace(uint32_t s){
    seed = s;
}

//An explicit PRNG is used so that seeds can be exported
uint32_t rand_replace(){
    seed *= 1103515245L;
    seed += 12345;
    seed &= 0x7fffffff;
    return seed & 0x3fffffff;
}

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

void put_pixel(Bitmap* canvas, Spr_params* params, int x, int y){
    int colors = canvas->colors;
    if(colors==0)colors = 1<<canvas->bps;
    float dist = hypot(x-canvas->width/2, y-canvas->height/2)/hypot(canvas->width/2,canvas->height/2);
    float prob = ((float)rand_replace())/rand_max_replace;
    prob = params->min + perlin_bias(perlin_gain(prob, params->gain), params->bias)/(params->max-params->min);
    uint16_t pix = 0;
    if(prob>=dist){
        pix = 1+(rand_replace()%(colors-1));
        set_pixel(canvas, x, y, pix);
    }
}

void copy_pixel(Bitmap* canvas, int dx, int dy, int sx, int sy){
    uint32_t src = get_pixel(canvas, sx, sy);
    set_pixel(canvas, dx, dy, src);
}

int count_surrounding(Bitmap* canvas, int x, int y){
    int ret = 0;
    ret += get_pixel(canvas, x-1, y)!=0;
    ret += get_pixel(canvas, x+1, y)!=0;
    ret += get_pixel(canvas, x, y-1)!=0;
    ret += get_pixel(canvas, x, y+1)!=0;
    ret += get_pixel(canvas, x+1, y+1)!=0;
    ret += get_pixel(canvas, x-1, y+1)!=0;
    ret += get_pixel(canvas, x+1, y-1)!=0;
    ret += get_pixel(canvas, x-1, y-1)!=0;
    return ret;
}

uint8_t random_neighbor(Bitmap* canvas, int x, int y){
    int dir = rand_replace()&3;
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
                if(count_surrounding(canvas, x, y)<3 && ((float)rand_replace())/rand_max_replace <= prob){
                    set_pixel(canvas, x, y, 0);
                }
            }else{ //should we add?
                if(count_surrounding(canvas, x, y)>=4 && ((float)rand_replace())/rand_max_replace <= prob){
                    set_pixel(canvas, x, y, random_neighbor(canvas, x, y));
                }
            }
        }
    }
}

void make_sprite(Bitmap* canvas, unsigned int seed, Spr_params* params){
    srand(seed);
    int mirX = params->x_prob >= ((float)rand_replace())/rand_max_replace;
    int mirY = params->y_prob >= ((float)rand_replace())/rand_max_replace;
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
    if(params->p_prob >= ((float)rand_replace())/rand_max_replace){
        for(int y=0; y<canvas->height; y++){
            for(int x=y; x<canvas->width; x++){
                if(x < canvas->height && y < canvas->width){
                    copy_pixel(canvas, x, y, y, x);
                }
            }
        }
    }
    if(params->n_prob >= ((float)rand_replace())/rand_max_replace){
        for(int y=0; y<canvas->height; y++){
            for(int x=canvas->width-y; x<canvas->width; x++){
                if(canvas->width-x < canvas->height && canvas->height-y < canvas->width){
                    copy_pixel(canvas, x, y, canvas->height-y, canvas->width-x);
                }
            }
        }
    }
}

Bitmap* random_doubling(Bitmap* src){
    Bitmap* dst = Bmp_empty(src->width*2, src->height*2, src->bps, src->colors);
    for(int x=0; x<src->width; x++){
        for(int y=0; y<src->height; y++){
            set_pixel(dst, x*2, y*2, get_pixel(src, x, y));
            set_pixel(dst, x*2+1, y*2, random_neighbor(src, x, y));
            set_pixel(dst, x*2, y*2+1, random_neighbor(src, x, y));
            set_pixel(dst, x*2+1, y*2+1, random_neighbor(src, x, y));
        }
    }
    if(src->bps<=8){
        int colors = src->colors;
        if(!colors)colors = 1<<src->bps;
        memcpy(dst->palette, src->palette, sizeof(uint32_t)*colors);
    }
    dst->R = dst->G = dst->B = 0;
    smooth(dst, 1);
    return dst;
}

int main(int argc, char** argv){
    unsigned int seed = time(NULL);
    int d = 32, colors = 8, bps = 8, doubling = 0;
    float gain = .5, bias = .5, min = 0, max = 1, m_x = .5, m_y = .5, m_p = .5, m_n = .5, filter = 1;
    char* filename = NULL;
    static struct option long_opts[] = {
        {"size",        1, NULL, 's'},
        {"colors",      1, NULL, 'c'},
        {"depth",       1, NULL, 'd'},
        {"gain",        1, NULL, 'g'},
        {"bias",        1, NULL, 'b'},
        {"min",         1, NULL, 'm'},
        {"max",         1, NULL, 'M'},
        {"mirror-x",    1, NULL, 'x'},
        {"mirror-y",    1, NULL, 'y'},
        {"mirror-pos",  1, NULL, 'p'},
        {"mirror-neg",  1, NULL, 'n'},
        {"output",      1, NULL, 'o'},
        {"seed",        1, NULL, 'r'},
        {"filter",      1, NULL, 'f'},
        {"upscales",    1, NULL, 'u'},
        {0, 0, 0, 0}
    };
    int c;
    while((c = getopt_long(argc, argv, "s:c:d:g:b:m:M:x:y:p:n:o:r:f:u:", long_opts, NULL)) != -1){
        switch(c){
            case 's':
                d = strtol(optarg, NULL, 10);break;
            case 'c':
                colors = strtol(optarg, NULL, 10);break;
            case 'd':
                bps = strtol(optarg, NULL, 10);break;
            case 'g':
                gain = strtod(optarg, NULL);break;
            case 'b':
                bias = strtod(optarg, NULL);break;
            case 'm':
                min = strtod(optarg, NULL);break;
            case 'M':
                max = strtod(optarg, NULL);break;
            case 'x':
                m_x = strtod(optarg, NULL);break;
            case 'y':
                m_y = strtod(optarg, NULL);break;
            case 'p':
                m_p = strtod(optarg, NULL);break;
            case 'n':
                m_n = strtod(optarg, NULL);break;
            case 'o':
                filename = malloc(strlen(optarg)+1);
                strcpy(filename, optarg);
                break;
            case 'r':
                seed = strtol(optarg, NULL, 10);break;
            case 'f':
                filter = strtod(optarg, NULL); break;
            case 'u':
                doubling = strtol(optarg, NULL, 10); break;
        }
    }
    if(filename==NULL){
        if(optind == argc){
            fprintf(stderr, "A filename must be provided with the -o or --output options!");
            return 1;
        }else{
            filename = malloc(strlen(argv[optind])+1);
            strcpy(filename, argv[optind]);
        }
    }
    srand_replace(seed);
    Bitmap* bmp = Bmp_empty(d, d, bps, colors);
    memset(bmp->bitmap, 0, (d*d*bps)/8);
    bmp->R = bmp->G = bmp->B = 0;
    if(bps<=8){
        bmp->palette[0] = 0;
        for(int i=1; i<colors; i++){
            float H = ((float)rand_replace())/rand_max_replace * M_PI * 2;
            float S = ((float)rand_replace())/rand_max_replace;
            float V = ((float)rand_replace())/rand_max_replace;
            bmp->palette[i] = HSV2RGB(H,S,V);
        }
    }
    Spr_params params = {bias, gain, min, max, filter, m_x, m_y, m_p, m_n};
    make_sprite(bmp, seed, &params);
    for(int i=0; i<doubling; i++){
        Bitmap* tmp = random_doubling(bmp);
        Bmp_free(bmp);
        bmp = tmp;
    }
    FILE* out = fopen(filename, "wb");
    Bmp_save(bmp, out);
    fclose(out);
    Bmp_free(bmp);
    free(filename);
    return 0;
}