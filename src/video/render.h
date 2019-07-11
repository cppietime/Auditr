#ifndef _H_RENDER
#define _H_RENDER

void mat4_mul(double*, double*, double*);
double dot_prod(double*, double*);
void cross_prod(double*, double*, double*);

typedef struct _canvas{
	unsigned char* pixels;
	float* depth;
	int width;
	int height;
} canvas;

typedef struct _face_3d{
	double a[3];
	double b[3];
	double c[3];
	double n[3];
} face_3d;

typedef struct _face_2d{
	double a[3];
	double b[3];
	double c[3];
	double v0[2];
	double v1[2];
	double d00,
		d11,
		d01;
} face_2d;

#endif