#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <sys/time.h>
#include "bmp.h"

static int big_endian = -1;

void test_endianness(){
    union {
        uint16_t two;
        uint8_t ones[2];
    } value;
    value.two = 1;
    if(value.ones[1] == 1){
        big_endian = 1;
    }else{
        big_endian = 0;
    }
}

void write_value(FILE* dst, uint64_t value, int len, int write_big){
    if(big_endian==-1){
        test_endianness();
    }
    union {
        uint64_t big;
        uint8_t smalls[8];
    } bytes;
    bytes.big = value;
    if(write_big){
        if(big_endian){
            for(int i=8-len; i<8; i++){
                fputc(bytes.smalls[i], dst);
            }
        }else{
            for(int i=len-1; i>=0; i--){
                fputc(bytes.smalls[i], dst);
            }
        }
    }else{
        if(big_endian){
            for(int i=7; i>=8-len; i--){
                fputc(bytes.smalls[i], dst);
            }
        }else{
            for(int i=0; i<len; i++){
                fputc(bytes.smalls[i], dst);
            }
        }
    }
}

void Bmp_validate_dib(Bmp_dib* dib, Bitmap* bitmap){
    dib->pix_width = bitmap->width;
    dib->pix_height = bitmap->height;
    if(!(bitmap->R | bitmap->G | bitmap->B)){
        dib->compression = BI_RGB;
    }else{
        dib->compression = BI_BITFIELDS;
    }
    if(bitmap->palette != NULL && bitmap->colors<(1<<bitmap->bps)){
        dib->colors = bitmap->colors;
    }else{
        dib->colors = 0;
    }
    dib->bitmap_size = bitmap->height * bitmap->row_bytes;
    dib->important_colors = 0;
    dib->xres = 300;
    dib->yres = 300;
    dib->bpp = bitmap->bps;
    if(dib->colors || dib->compression) {
        dib->dib_size = BITMAPINFOHEADER;
    }else{
        dib->dib_size = BITMAPCOREHEADER;
    }
    printf("DIB val'd\n");
}

void Bmp_validate_header(Bmp_header* header, Bmp_dib* dib, Bitmap* bitmap){
    header->magic[0] = 'B';
    header->magic[1] = 'M';
    header->file_size = 14 + dib->dib_size + dib->bitmap_size;
    if(dib->compression == BI_BITFIELDS){
        header->file_size += 3*dib->bpp/8;
    }
    if(dib->bpp<=8){
        int colors = dib->colors;
        if(colors == 0){
            colors = 1<<dib->bpp;
        }
        header->file_size += colors*4;
    }
    header->img_offset = header->file_size - dib->bitmap_size;
    printf("Header val'd\n");
}

void Bmp_save(Bitmap* bitmap, FILE* dst){
    Bmp_validate_dib(bitmap->dib, bitmap);
    Bmp_validate_header(&bitmap->header, bitmap->dib, bitmap);
    printf("Type: %d\n",bitmap->dib->compression);
    //write header
    fwrite(bitmap->header.magic, 1, 2, dst); //"BM"
    write_value(dst, bitmap->header.file_size, 4, 0); //file size in bytes
    write_value(dst, 0, 2, 0); //reserved 1
    write_value(dst, 0, 2, 0); //reserved 2
    write_value(dst, bitmap->header.img_offset, 4, 0); //offset of bitmap image
    printf("Wrote header @%d\n",ftell(dst));
    //write DIB
    if(bitmap->dib->dib_size == BITMAPCOREHEADER){
        write_value(dst, BITMAPCOREHEADER,          4, 0);
        write_value(dst, bitmap->dib->pix_width,    2, 0);
        write_value(dst, bitmap->dib->pix_height,   2, 0);
        write_value(dst, 1, 2, 0);
        write_value(dst, bitmap->dib->bpp,          2, 0);
    }else if(bitmap->dib->dib_size == BITMAPINFOHEADER){
        int dib_size = BITMAPINFOHEADER;
        printf("Dib size: %d\n",dib_size);
        write_value(dst, dib_size,                  4, 0);
        write_value(dst, bitmap->dib->pix_width,    4, 0);
        write_value(dst, bitmap->dib->pix_height,   4, 0);
        write_value(dst, 1, 2, 0);
        write_value(dst, bitmap->dib->bpp,          2, 0);
        write_value(dst, bitmap->dib->compression,  4, 0);
        write_value(dst, bitmap->dib->bitmap_size,  4, 0);
        write_value(dst, bitmap->dib->xres,         4, 0);
        write_value(dst, bitmap->dib->yres,         4, 0);
        write_value(dst, bitmap->dib->colors,       4, 0);
        write_value(dst, bitmap->dib->important_colors, 4, 0);
    }
    printf("Wrote DIB @%d\n",ftell(dst));
    //write color bitmasks
    if(bitmap->dib->compression == BI_BITFIELDS || bitmap->dib->compression == BI_ALPHABITFIELDS){
        write_value(dst, bitmap->R,                 4, 0);
        write_value(dst, bitmap->G,                 4, 0);
        write_value(dst, bitmap->B,                 4, 0);
    }
    //write palette
    if(bitmap->dib->bpp <= 8 && bitmap->palette != NULL){
        int colors = bitmap->dib->colors;
        if(colors==0){
            colors = 1<<bitmap->dib->bpp;
        }
        for(int i=0; i<colors; i++){
            write_value(dst, bitmap->palette[i],    4, 0);
        }
    }
    printf("Offset: %d\n", ftell(dst));
    printf("Image offset: %d\n", bitmap->header.img_offset);
    //write bitmap data
    for(int y=0; y<bitmap->dib->pix_height; y++){
        if(bitmap->dib->bpp <= 8){
            fwrite(bitmap->bitmap + y*bitmap->row_bytes, bitmap->row_bytes, 1, dst);
        }else{
            int bytes = 0;
            for(int x=0; x<bitmap->dib->pix_width; x++){
                uint64_t value = 0;
                memcpy(&value, bitmap->bitmap + y*bitmap->row_bytes + x*(bitmap->dib->bpp/8), bitmap->dib->bpp/8);
                write_value(dst, value, bitmap->dib->bpp/8, 0);
                bytes += bitmap->dib->bpp/8;
            }
            for(; bytes<bitmap->row_bytes; bytes++){
                fputc(0, dst);
            }
        }
    }
}

Bitmap* Bmp_empty(int width, int height, int bps, int colors){
    Bitmap* ret = malloc(sizeof(Bitmap));
    ret->width = width;
    ret->height = height;
    ret->bps = bps;
    ret->colors = colors;
    ret->dib = malloc(sizeof(Bmp_dib));
    if(bps<=8){
        if(colors==0){
            colors = 1<<bps;
        }
        ret->palette = malloc(sizeof(uint32_t)*colors);
    }else{
        ret->palette = NULL;
    }
    int row_bytes = (width*bps+7)>>3;
    row_bytes = (row_bytes+3)&~3;
    ret->row_bytes = row_bytes;
    ret->bitmap = malloc(row_bytes*height);
    return ret;
}

void Bmp_free(Bitmap* bitmap){
    free(bitmap->dib);
    if(bitmap->palette!=NULL){
        free(bitmap->palette);
    }
    free(bitmap->bitmap);
    free(bitmap);
}