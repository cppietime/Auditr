#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <getopt.h>
#include "render.h"
#include "pbm.h"

#define MAX(x,y) (x)>(y)?(x):(y)
#define MIN(x,y) (x)<=(y)?(x):(y)

double ihat[] = {1,0,0};
double jhat[] = {0,1,0};
double khat[] = {0,0,1};

void print_mat(double* d){
	int x,y;
	for(y=0; y<4; y++){
		for(x=0; x<4; x++){
			printf("%lf\t",d[y*4+x]);
		}puts("");
	}
}

void mat4_mul(double* dst, double* left, double* right){
	static double tmp[16];
	memset(tmp, 0, sizeof(double)*16);
	int i,j,k;
	for(i=0; i<4; i++){
		for(k=0; k<4; k++){
			for(j=0; j<4; j++){
				tmp[i*4+j] += left[i*4+k]*right[k*4+j];
			}
		}
	}
	memcpy(dst,tmp,sizeof(double)*16);
}

void vec4_mul(double* dst, double* mat, double* vec){
	static double tmp[4];
	int i,j;
	for(i=0; i<4; i++){
		tmp[i]=0;
		for(j=0; j<4; j++){
			double comp = 1;
			if(j<3)
				comp = vec[j];
			tmp[i] += mat[i*4+j]*comp;
		}
	}
	for(i=0; i<3; i++)
		tmp[i]/=tmp[3];
	memcpy(dst,tmp,sizeof(double)*3);
}

void vec_sub(double* dst, double* a, double* b, int n){
	int i;
	for(i=0; i<n; i++)
		dst[i] = a[i] - b[i];
}

void vec_add(double* dst, double* a, double* b, int n){
	int i;
	for(i=0; i<n; i++)
		dst[i] = a[i] + b[i];
}

double dot_prod(double* a, double* b, int n){
	double ret = 0;
	int i;
	for(i=0; i<n; i++)
		ret += a[i]*b[i];
	return ret;
}

void cross_prod(double* dst, double* a, double* b){
	static double tmp[3];
	tmp[0] = a[1]*b[2]-a[2]*b[1];
	tmp[1] = a[2]*b[0]-a[0]*b[2];
	tmp[2] = a[0]*b[1]-a[1]*b[0];
	memcpy(dst,tmp,sizeof(double)*3);
}

void rot_matrix(double* dst, double* axis, double rads){
	memset(dst, 0, sizeof(double)*16);
	double r = cos(rads/2),
		v = sin(rads/2),
		i = v*axis[0],
		j = v*axis[1],
		k = v*axis[2];
	dst[0] = 1-2*(k*k+j*j);
	dst[1] = 2*(i*j-k*r);
	dst[2] = 2*(i*k+j*r);
	dst[4] = 2*(i*j+k*r);
	dst[5] = 1-2*(i*i+k*k);
	dst[6] = 2*(k*j-i*r);
	dst[8] = 2*(i*k-j*r);
	dst[9] = 2*(j*k+i*r);
	dst[10] = 1-2*(i*i+j*j);
	dst[15] = 1;
}

void trans_matrix(double* dst, double* pos){
	memset(dst, 0, sizeof(double)*16);
	dst[0] = 1;
	dst[5] = 1;
	dst[10] = 1;
	dst[3] = pos[0];
	dst[7] = pos[1];
	dst[11] = pos[2];
	dst[15] = 1;
}

void cam_matrix(double* dst, double* pos, double* angles){
	static double tmp[16];
	rot_matrix(dst, jhat, angles[1]);
	rot_matrix(tmp, ihat, angles[0]);
	mat4_mul(dst, dst, tmp);
	rot_matrix(tmp, khat, angles[2]);
	mat4_mul(dst, dst, tmp);
	trans_matrix(tmp, pos);
	mat4_mul(dst, tmp, dst);
}

void proj_matrix(double* dst, double width, double height, double near, double far){
	memset(dst, 0, sizeof(double)*16);
	dst[0] = -near/width;
	dst[5] = -near/height;
	dst[10] = 1/(far-near);
	dst[11] = near/(near-far);
	dst[14] = -1;
}

void cache_face(face_2d* face){
	vec_sub(face->v0,face->b,face->a,2);
	vec_sub(face->v1,face->c,face->a,2);
	face->d00 = dot_prod(face->v0,face->v0,2);
	face->d01 = dot_prod(face->v0,face->v1,2);
	face->d11 = dot_prod(face->v1,face->v1,2);
	face->denom = 1.0/(face->d00*face->d11-face->d01*face->d01);
}

void to_bary(double* dst, face_2d* face, double* pos){
	double v2[2];
	vec_sub(v2,pos,face->a,2);
	double d20 = dot_prod(v2,face->v0,2);
	double d21 = dot_prod(v2,face->v1,2);
	dst[1] = (d20*face->d11 - d21*face->d01)*face->denom;
	dst[2] = (d21*face->d00 - d20*face->d01)*face->denom;
	dst[0] = 1-dst[1]-dst[2];
}

void bary_sum(double* dst, double* bary, double* a, double* b, double* c, int n){
	int i;
	for(i=0; i<n; i++){
		dst[i] = a[i]*bary[0] + b[i]*bary[1] + c[i]*bary[2];
	}
}

static double cam_proj_mat[16];

void set_view(double* cam, double* proj){
	mat4_mul(cam_proj_mat, proj, cam);
}

void render_solid(solid* object, canvas* screen, double* light){
	static double world_mat[16];
	static face_2d face;
	mat4_mul(world_mat, cam_proj_mat, object->transform);
	int face_id;
	for(face_id = 0; face_id<object->face_num; face_id++){
		face_3d* obj_face = object->faces[face_id];
		double mul = 1;
		if(light != NULL)
			mul = fabs(dot_prod(obj_face->n, light, 3));
		vec4_mul(face.a, world_mat, obj_face->verts[0]->xyz);
		vec4_mul(face.b, world_mat, obj_face->verts[1]->xyz);
		vec4_mul(face.c, world_mat, obj_face->verts[2]->xyz);
		cache_face(&face);
		if(fabs(1.0/face.denom)<1e-16)
			continue;
		double minZ = MIN(face.a[2],MIN(face.b[2],face.c[2])),
			maxZ = MAX(face.a[2],MAX(face.b[2],face.c[2]));
		if(minZ>1 || maxZ<0)
			continue;
		double minX = MIN(face.a[0], MIN(face.b[0], face.c[0])),
			maxX = MAX(face.a[0], MAX(face.b[0], face.c[0])),
			minY = MIN(face.a[1], MIN(face.b[1], face.c[1])),
			maxY = MAX(face.a[1], MAX(face.b[1], face.c[1]));
		if(minX>1 || minY>1 || maxX<-1 || maxY<-1)
			continue;
		if(minX<-1)
			minX=-1;
		if(minY<-1)
			minY=-1;
		if(maxX>1)
			maxX=1;
		if(maxY>1)
			maxY=1;
		int x0 = (minX/2+.5)*screen->width,
			x1 = (maxX/2+.5)*screen->width,
			y0 = (minY/2+.5)*screen->height,
			y1 = (maxY/2+.5)*screen->height;
		int x,y;
		double bary[3];
		double xy[2];
		double color[3];
		for(x=x0; x<=x1; x++){
			xy[0]=2.0*x/screen->width-1;
			for(y=y0; y<=y1; y++){
				xy[1]=2.0*y/screen->height-1;
				to_bary(bary,&face,xy);
				if(bary[0]<0 || bary[0]>1 || bary[1]<0 || bary[1]>1 || bary[2]<0 || bary[2]>1)
					continue;
				double z;
				bary_sum(&z, bary, face.a+2, face.b+2, face.c+2, 1);
				if(z<0 || z>1)
					continue;
				int index = (screen->height-1-y)*screen->width+x;
				if(z>screen->depth[index])
					continue;
				screen->depth[index] = z;
				bary_sum(color, bary, obj_face->verts[0]->rgb, obj_face->verts[1]->rgb, obj_face->verts[2]->rgb, 3);
				int i;
				for(i=0; i<3; i++){
					if(color[i]<0)color[i]=0;
					else if(color[i]>1)color[i]=1;
				}
				screen->pixels[index*3] = color[0]*255*mul;
				screen->pixels[index*3+1] = color[1]*255*mul;
				screen->pixels[index*3+2] = color[2]*255*mul;
			}
		}
	}
}

void render_pyro(pyro* tech, canvas* screen){
	if(tech->children!=NULL){
		int i;
		for(i=0; i<tech->spawn; i++){
			render_pyro(tech->children[i], screen);
		}
	}
	if(tech->timer<=0){
		return;
	}
	double pos[3];
	vec4_mul(pos, cam_proj_mat, tech->pos);
	if(pos[2]>1 || pos[2]<0)
		return;
	if(pos[0]<-1-tech->radius || pos[0]>1+tech->radius || pos[1]<-1-tech->radius || pos[1]>1+tech->radius)
		return;
	int x0 = MAX(0,(pos[0]-tech->radius+1.0)*screen->width/2),
		x1 = MIN(screen->width-1,(pos[0]+tech->radius+1.0)*screen->width/2),
		y0 = MAX(0,(pos[1]-tech->radius+1.0)*screen->width/2),
		y1 = MIN(screen->height-1,(pos[1]+tech->radius+1.0)*screen->width/2);
	// printf("%d - %d, %d - %d\n",x0, x1, y0, y1);
	int x,y;
	double xy[2];
	double dist[2];
	double rad3d = tech->radius / (1+pos[2]);
	for(y=y0; y<=y1; y++){
		xy[1] = y*2.0/screen->width-1.0;
		for(x=x0; x<=x1; x++){
			int index = (screen->height-1-y)*screen->width+x;
			if(pos[2]>screen->depth[index])
				continue;
			xy[0] = x*2.0/screen->width-1.0;
			vec_sub(dist,xy,pos,2);
			double rad2 = dot_prod(dist,dist,2);
			// printf("(%lf,%lf)-(%lf,%lf)->%lf\n",pos[0],pos[1],xy[0],xy[1],rad2);
			if(rad2>rad3d*rad3d)
				continue;
			screen->pixels[index*3] = tech->color[0]*255;
			screen->pixels[index*3+1] = tech->color[1]*255;
			screen->pixels[index*3+2] = tech->color[2]*255;
		}
	}
}

int update_pyro(pyro* tech){
	int ret = 0;
	if(tech->children!=NULL){
		int i;
		for(i=0; i<tech->spawn; i++){
			ret |= update_pyro(tech->children[i]);
		}
	}
	if(tech->timer<=0)
		return ret;
	vec_add(tech->pos, tech->pos, tech->vel, 3);
	// printf("Pos %lf %lf %lf\n", tech->pos[0], tech->pos[1], tech->pos[2]);
	tech->timer--;
	if(tech->timer==0 && tech->spawn>0 && tech->level>0){
		// printf("Spawning children\n");
		tech->children = malloc(sizeof(pyro*)*tech->spawn);
		double newrad = tech->radius;
		double mag = sqrt(dot_prod(tech->vel,tech->vel,3));
		// printf("Vel mag %lf\n",mag);
		int newtime = (int)(1.0/mag);
		float newcol[] = {(float)rand()/RAND_MAX, (float)rand()/RAND_MAX, (float)rand()/RAND_MAX};
		// printf("Color %f %f %f\n", newcol[0], newcol[1], newcol[2]);
		int i;
		for(i=0; i<tech->spawn; i++){
			double theta = rand()*2.0*M_PI/RAND_MAX;
			double phi = rand()*M_PI/2/RAND_MAX;
			tech->children[i] = malloc(sizeof(pyro));
			*(tech->children[i]) = (pyro){
				{tech->pos[0], tech->pos[1], tech->pos[2]},
				{mag*cos(phi)*cos(theta), mag*cos(phi)*sin(theta), mag*sin(theta)},
				newrad,
				newtime,
				tech->spawn,
				tech->level-1,
				{newcol[0], newcol[1], newcol[2]},
				NULL
			};
		}
	}
	return 1;
}

void free_pyro(pyro* tech){
	if(tech->children!=NULL){
		int i;
		for(i=0; i<tech->spawn; i++){
			free_pyro(tech->children[i]);
		}
		free(tech->children);
	}
	free(tech);
}

int main(int argc, char** argv){
	int frames = 90;
	char path[128] = "";
	int i;
	while((i=getopt(argc,argv,"f:o:"))!=-1){
		switch(i){
			case 'f':
				frames = atoi(optarg);break;
			case 'o':
				strcpy(path,optarg);
		}
	}
	int plen = strlen(path);
	if(plen>0 && path[plen-1]!='/')
		path[plen++] = '/';
	srand(time(NULL));
	canvas screen;
	screen.width = 1280;
	screen.height = 720;
	screen.pixels = calloc(1,1280*720*3);
	screen.depth = malloc(sizeof(float)*1280*720);
	for(i=0; i<1280*720; i++){
		screen.depth[i] = INFINITY;
	}
	double pos[] = {0, 0, 0};
	double angles[] = {0, 0, 0};
	double cam[16];
	cam_matrix(cam,pos,angles);
	double proj[16];
	proj_matrix(proj,1,1,-1,-3);
	print_mat(proj);
	set_view(cam,proj);
	int j;
	pyro* tech = NULL;
	char name[256];
	for(i=0; i<frames; i++){
		if(tech==NULL){
			tech = malloc(sizeof(pyro));
			double hspd = 1.0/100;
			*tech = (pyro){{rand()*.5/RAND_MAX-.25, -1, -2}, {rand()*.2/RAND_MAX/30, hspd, 0}, rand()*.05/RAND_MAX, (int)(.5/hspd),
				rand()%8, rand()%5, {rand()*1.0/RAND_MAX, rand()*1.0/RAND_MAX, rand()*1.0/RAND_MAX}, NULL};
			printf("Velocity %lf %lf %lf\n", tech->vel[0], tech->vel[1], tech->vel[2]);
		}
		memset(screen.pixels,0,1280*720*3);
		for(j=0; j<1280*720; j++){
			screen.depth[j] = INFINITY;
		}
		render_pyro(tech, &screen);
		if(update_pyro(tech)==0){
			free_pyro(tech);
			tech=NULL;
		}
		strcpy(name,path);
		sprintf(name+plen, "%04d.ppm", i);
		FILE* file_out = fopen(name,"wb");
		save_image(file_out,1280,720,screen.pixels);
		fclose(file_out);
		printf("Wrote frame %d to %s\n",i,name);
	}
	if(tech!=NULL)
		free_pyro(tech);
	printf("Complete\n");
	free(screen.pixels);
	free(screen.depth);
	return 0;
}

#undef MAX
#undef MIN