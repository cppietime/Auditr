#ifndef _H_SPRITEGEN
#define _H_SPRITEGEN

#include <stdint.h>
#include "bmp.h"

#define MIRROR_X 1
#define MIRROR_Y 2
#define MIRROR_POS 4
#define MIRROR_NEG 8

typedef struct _Spr_params{
    float bias;
    float gain;
    float min;
    float max;
    float sm_prob;
    float x_prob;
    float y_prob;
    float p_prob;
    float n_prob;
    int flags;
} Spr_params;

void make_sprite(Bitmap*, unsigned int, Spr_params*);

#endif