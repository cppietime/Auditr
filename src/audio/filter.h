#ifndef _H_FILTER
#define _H_FILTER

#include <complex.h>

//Polynomial with real coefficients
typedef struct _rfunc{
	int order;
	double* coefs;
} rfunc;

void rfunc_free(rfunc*);
void rfunc_print(rfunc*);
rfunc* rfunc_mul(rfunc*, rfunc*);
//Construct rfunc from two conjugate roots
rfunc* rfunc_conj(double,double);
//Contruct rfunc with conjugate roots
rfunc* for_roots_r(double*,int);
complex double rfunc_calc(rfunc*,complex double);
//rfunc for poles at frequencies given
rfunc* all_poles(double,double,double*,int);
//same as all_poles but with independent magnitudes
rfunc* point_poles(double,double*,double*,int);
//find magnitude of highest response for rfunc
double rfunc_max_response(rfunc*,rfunc*,double,double*,int);
//normalize rfunc by first coefficient
void rfunc_divhead(rfunc*);
//nomralize by given factor
void rfunc_normalize(rfunc*,double);
//apply filter from one array to another
void filter(double*,double*,int,rfunc*,rfunc*);
//apply filter in place
void filter_in_place(double*,int,rfunc*,rfunc*);

typedef double (*wavegen)(double);

typedef struct _filter_one{
    double* zeros;
    double* poles;
    int* zero_types;
    int* pole_types;
} filter_one;

#endif