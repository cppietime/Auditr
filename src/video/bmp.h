#ifndef _BMP_H
#define _BMP_H

#include <stdint.h>

typedef struct _Bmp_header{
    char magic[2];
    uint32_t file_size;
    uint32_t img_offset;
} Bmp_header;

#define BITMAPCOREHEADER 12
#define BITMAPINFOHEADER 40

#define BI_RGB 0
#define BI_BITFIELDS 3
#define BI_ALPHABITFIELDS 6

typedef struct _Bmp_dib{
    uint32_t dib_size;
    int32_t pix_width;
    int32_t pix_height;
    uint32_t compression;
    uint32_t bitmap_size;
    int32_t xres;
    int32_t yres;
    uint32_t colors;
    uint32_t important_colors;
    uint16_t bpp;
} Bmp_dib;

typedef struct _Bitmap{
    Bmp_dib* dib;
    uint32_t* palette;
    void* bitmap;
    Bmp_header header;
    uint32_t R, G, B;
    int32_t width, height;
    uint32_t row_bytes;
    uint16_t colors;
    uint8_t bps;
} Bitmap;

void Bmp_validate_dib(Bmp_dib*, Bitmap*);
void Bmp_validate_header(Bmp_header*, Bmp_dib*, Bitmap*);
void Bmp_save(Bitmap*, FILE*);

Bitmap* Bmp_empty(int, int, int, int);
void Bmp_free(Bitmap*);

#endif