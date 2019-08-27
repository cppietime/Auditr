/*
Library to randomly generated small(ish) paletted sprites.
*/

#ifndef _H_SPRITEGEN
#define _H_SPRITEGEN

#include <stdint.h>
#include "bmp.h"

typedef struct _Spr_params{
    float bias; //Perlin bias, --bias or -b
    float gain; //Perlin gain --gain or -g
    float min; //Minimum probability for pixel --min or -m
    float max; //Maximum probability for pixel --max or -M
    float sm_prob; //Chance to remove/fill in isolated pixel --filter or -f
    float x_prob; //Chance to mirror about X axis --mirror-x or -x
    float y_prob; //Chance to mirror about Y axis --mirror-y or -y
    float p_prob; //Chance to mirror about Y=X --mirror-pos or -p
    float n_prob; //Chance to mirror about Y=-X --mirror-neg or -n
} Spr_params;

/*Generate a sprite and save it to a Bitmap
The Bitmap must already be allocated, and it is best
to use this for paletted Bitmaps. 2-bit Bitmaps
don't really work.
*/
void make_sprite(Bitmap*, unsigned int, Spr_params*);

Bitmap* random_doubling(Bitmap*);

#endif