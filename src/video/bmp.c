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

uint64_t read_value(FILE* src, int len, int read_big){
    if(big_endian==-1){
        test_endianness();
    }
    union {
        uint64_t big;
        uint8_t smalls[8];
    } bytes;
    bytes.big = 0;
    if(read_big){
        if(big_endian){
            for(int i=8-len; i<8; i++){
                bytes.smalls[i] = fgetc(src);
            }
        }else{
            for(int i=len-1; i>=0; i--){
                bytes.smalls[i] = fgetc(src);
            }
        }
    }else{
        if(big_endian){
            for(int i=7; i>=8-len; i--){
                bytes.smalls[i] = fgetc(src);
            }
        }else{
            for(int i=0; i<len; i++){
                bytes.smalls[i] = fgetc(src);
            }
        }
    }
    return bytes.big;
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
}

void Bmp_save(Bitmap* bitmap, FILE* dst){
    Bmp_validate_dib(bitmap->dib, bitmap);
    Bmp_validate_header(&bitmap->header, bitmap->dib, bitmap);
    //write header
    fwrite(bitmap->header.magic, 1, 2, dst); //"BM"
    write_value(dst, bitmap->header.file_size, 4, 0); //file size in bytes
    write_value(dst, 0, 2, 0); //reserved 1
    write_value(dst, 0, 2, 0); //reserved 2
    write_value(dst, bitmap->header.img_offset, 4, 0); //offset of bitmap image
    //write DIB
    if(bitmap->dib->dib_size == BITMAPCOREHEADER){
        write_value(dst, BITMAPCOREHEADER,          4, 0);
        write_value(dst, bitmap->dib->pix_width,    2, 0);
        write_value(dst, bitmap->dib->pix_height,   2, 0);
        write_value(dst, 1, 2, 0);
        write_value(dst, bitmap->dib->bpp,          2, 0);
    }else if(bitmap->dib->dib_size == BITMAPINFOHEADER){
        int dib_size = BITMAPINFOHEADER;
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
    //write color bitmasks
    if(bitmap->dib->compression == BI_BITFIELDS){
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

Bitmap* Bmp_load(FILE* src){
    Bitmap* ret = malloc(sizeof(Bitmap));
    fread(ret->header.magic, 1, 2, src);
    ret->header.file_size = read_value(src, 4, 0);
    fseek(src, 4, SEEK_CUR);
    ret->header.img_offset = read_value(src, 4, 0);
    ret->dib = malloc(sizeof(Bmp_dib));
    ret->dib->dib_size = read_value(src, 4, 0);
    if(ret->dib->dib_size == BITMAPCOREHEADER){
        ret->width = read_value(src, 2, 0);
        ret->height = read_value(src, 2, 0);
        fseek(src, 2, SEEK_CUR);
        ret->bps = read_value(src, 2, 0);
        ret->row_bytes = (ret->width*ret->bps+7)>>3;
        ret->row_bytes = (ret->row_bytes+3)&~3;
        ret->dib->bitmap_size = ret->height*ret->row_bytes;
        ret->colors = 0;
        ret->dib->important_colors = 0;
        ret->dib->compression = BI_RGB;
    }else{
        ret->width = read_value(src, 4, 0);
        ret->height = read_value(src, 4, 0);
        fseek(src, 2, SEEK_CUR);
        ret->bps = read_value(src, 2, 0);
        ret->dib->compression = read_value(src, 4, 0);
        ret->dib->bitmap_size = read_value(src, 4, 0);
        ret->dib->xres = read_value(src, 4, 0);
        ret->dib->yres = read_value(src, 4, 0);
        ret->colors = read_value(src, 4, 0);
        ret->dib->important_colors = read_value(src, 4, 0);
    }
    ret->dib->colors = ret->colors;
    ret->dib->bpp = ret->bps;
    ret->dib->pix_width = ret->width;
    ret->dib->pix_height = ret->height;
    if(ret->dib->compression == BI_BITFIELDS){
        ret->R = read_value(src, 4, 0);
        ret->G = read_value(src, 4, 0);
        ret->B = read_value(src, 4, 0);
    }else{
        ret->R = ret->G = ret->B = 0;
    }
    if(ret->bps<=8){
        int cols = ret->colors;
        if(cols==0){
            cols = 1<<ret->bps;
        }
        ret->palette = malloc(sizeof(uint32_t)*cols);
        for(int i=0; i<cols; i++){
            ret->palette[i] = read_value(src, 4, 0);
        }
    }
    ret->bitmap = malloc(ret->dib->bitmap_size);
    for(int y=0; y<ret->height; y++){
        if(ret->bps<=8){
            fread(ret->bitmap + y*ret->row_bytes, ret->row_bytes, 1, src);
        }else{
            int bytes = 0;
            for(int i=0; i<ret->width; i++){
                uint64_t value = read_value(src, ret->bps/8, 0);
                memcpy(ret->bitmap + y*ret->row_bytes + (i*ret->bps)/8, &value, ret->bps/8);
                bytes += ret->bps/8;
            }
            fread(ret->bitmap+y*ret->row_bytes + bytes, ret->row_bytes-bytes, 1, src);
        }
    }
    return ret;
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

uint32_t get_pixel(Bitmap* bitmap, int x, int y){
    if(x<0 || y<0 || x>=bitmap->width || y>=bitmap->height){
        return 0;
    }
    int bps = bitmap->bps;
    if(bps<8){
        int chunks = 8/bps;
        uint16_t bitmask = 0xffff<<(bps);
        bitmask = ~bitmask;
        int index = y*bitmap->row_bytes + (x*bps)/8;
        uint8_t* pixdata = (uint8_t*)bitmap->bitmap;
        uint16_t ret = pixdata[index];
        int into = chunks - 1 - (x%chunks);
        ret >>= bps * into;
        return ret&bitmask;
    }
    void* addr = bitmap->bitmap + y*bitmap->row_bytes + x*(bps/8);
    uint32_t ret = 0;
    memcpy(&ret, addr, bps/8);
    return ret;
}

void set_pixel(Bitmap* bitmap, int x, int y, uint32_t pix){
    int bps = bitmap->bps;
    if(bps<8){
        int chunks = 8/bps;
        uint16_t bitmask = 0xffff<<(bps);
        bitmask = ~bitmask;
        pix &= bitmask;
        int index = y*bitmap->row_bytes + (x*bps)/8;
        uint8_t* pixdata = (uint8_t*)bitmap->bitmap;
        int into = chunks - 1 - (x%chunks);
        uint8_t mask = bitmask << (bps*into);
        pixdata[index] &= ~mask;
        pixdata[index] |= pix<<(bps*into);
    }else{
        void* addr = bitmap->bitmap + y*bitmap->row_bytes + x*(bps/8);
        memcpy(addr, &pix, bps/8);
    }
}

void Bmp_dump(FILE* dst, Bitmap* bitmap){
    fprintf(dst, "Bitmap with bpp: %d\nSize: %d, %d\nCompression: %d\nImage size: %d\nPitch: %d\n",bitmap->bps,
        bitmap->width, bitmap->height, bitmap->dib->compression, bitmap->dib->bitmap_size, bitmap->row_bytes);
    if(bitmap->dib->compression == BI_BITFIELDS){
        fprintf(dst, "R: %p, G: %p, B: %p\n", bitmap->R, bitmap->G, bitmap->B);
    }
    if(bitmap->bps<=8){
        int cols = bitmap->colors;
        if(cols==0)cols = 1<<bitmap->bps;
        for(int i=0; i<cols; i++){
            fprintf(dst, "Palette[%d]: %p\n", i, bitmap->palette[i]);
        }
    }
}

uint32_t HSV2RGB(float H, float S, float V){
    float hi = V;
    float lo = V*(1-S);
    while(H<0)H+=M_PI*2;
    while(H>=M_PI*2)H-=M_PI*2;
    float weight = fmod(H, M_PI/3) * 3.0/M_PI;
    int step = (int)(floor(H*3.0/M_PI)+.5);
    float med = (step&1)?hi*weight + lo*(1.0-weight):lo*weight + hi*(1.0-weight);
    int r, g, b;
    switch(step){
        case 0:
            r = hi*255;
            b = lo*255;
            g = med*255;
            break;
        case 1:
            g = hi*255;
            b = lo*255;
            r = med*255;
            break;
        case 2:
            g = hi*255;
            r = lo*255;
            b = med*255;
            break;
        case 3:
            b = hi*255;
            r = lo*255;
            g = med*255;
            break;
        case 4:
            b = hi*255;
            g = lo*255;
            r = med*255;
            break;
        case 5:
            r = hi*255;
            g = lo*255;
            b = med*255;
            break;
    }
    r &= 0xff;
    g &= 0xff;
    b &= 0xff;
    return (r<<16)|(g<<8)|b;
}