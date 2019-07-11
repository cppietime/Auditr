#include <stdio.h>
#include "pbm.h"

void save_image(FILE* out, int width, int height, char* data){
	fprintf(out, "P6\n%d %d\n255\n", width, height);
	fwrite(data, 1, width*height*3, out);
}