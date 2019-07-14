#ifndef _H_RENDER
#define _H_RENDER

void mat4_mul(double*, double*, double*);
void vec4_mul(double*, double*, double*);
void vec_sub(double*, double*, double*, int);
void vec_add(double*, double*, double*, int);
double dot_prod(double*, double*, int);
void cross_prod(double*, double*, double*);

void rot_matrix(double*, double*, double);
void trans_matrix(double*, double*);

void cam_matrix(double*, double*, double*);
void proj_matrix(double*, double, double, double, double);

typedef struct _canvas{
	unsigned char* pixels;
	float* depth;
	int width;
	int height;
} canvas;

typedef struct _point{
	double xyz[3];
	double* rgb;
} point;

typedef struct _face_3d{
	point* verts[3];
	double n[3];
} face_3d;

typedef struct _solid{
	face_3d** faces;
	int face_num;
	double transform[16];
} solid;

typedef struct _face_2d{
	double a[3];
	double b[3];
	double c[3];
	double v0[3];
	double v1[3];
	double d00,
		d11,
		d01,
		denom;
} face_2d;

void cache_face(face_2d*);
void to_bary(double*, face_2d*, double*);
void bary_sum(double*, double*, double*, double*, double*, int);

void set_view(double*, double*);
void render_solid(solid*, canvas*, double*);

typedef struct _pyro{
	double pos[3];
	double vel[3];
	double radius;
	int timer;
	int spawn;
	int level;
	float color[3];
	struct _pyro** children;
} pyro;

void render_pyro(pyro*, canvas*);
int update_pyro(pyro*);
void free_pyro(pyro*);

#endif