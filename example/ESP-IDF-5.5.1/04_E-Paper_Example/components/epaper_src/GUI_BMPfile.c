/*****************************************************************************
* | File      	:   GUI_BMPfile.h
* | Author      :   WavesGUI_ReadBmp_RGB_6Colorhare team
* | Function    :   Hardware underlying interface
* | Info        :
*                Used to shield the underlying layers of each master
*                and enhance portability
*----------------
* |	This version:   V2.3
* | Date        :   2022-07-27
* | Info        :   
* -----------------------------------------------------------------------------
* V2.3(2022-07-27):
* 1.Add GUI_ReadBmp_RGB_4Color()
* V2.2(2020-07-08):
* 1.Add GUI_ReadBmp_RGB_7Color()
* V2.1(2019-10-10):
* 1.Add GUI_ReadBmp_4Gray()
* V2.0(2018-11-12):
* 1.Change file name: GUI_BMP.h -> GUI_BMPfile.h
* 2.fix: GUI_ReadBmp()
*   Now Xstart and Xstart can control the position of the picture normally,
*   and support the display of images of any size. If it is larger than
*   the actual display range, it will not be displayed.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/

#include "GUI_BMPfile.h"
#include "GUI_Paint.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>	//exit()
#include <string.h> //memset()
#include <limits.h>
#include <math.h> //memset()
#include <stdio.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_heap_caps.h"

static const char *TAG = "GUI_BMPfile";

extern const uint8_t peacock_bmp_start[] asm("_binary_peacock_bmp_start");
extern const uint8_t peacock_bmp_end[]   asm("_binary_peacock_bmp_end");

typedef struct {
    FILE *f;
    const uint8_t *mem;
    size_t mem_size;
    size_t mem_pos;
} bmp_src_t;

static const char *bmp_basename(const char *path)
{
    const char *base = path;
    for (const char *cursor = path; cursor && *cursor; cursor++) {
        if ((*cursor == '/') || (*cursor == '\\')) {
            base = cursor + 1;
        }
    }
    return base;
}

static const uint8_t k_palette_rgb[6][3] = {
    {  0,   0,   0},
    {255, 255, 255},
    {255, 255,   0},
    {255,   0,   0},
    {  0,   0, 255},
    {  0, 255,   0},
};
static const uint8_t k_palette_idx[6] = {0, 1, 2, 3, 5, 6};

static uint8_t rgb_to_epd6(uint8_t r, uint8_t g, uint8_t b)
{
    int best_i = 1;
    int best_d = INT_MAX;
    for (int i = 0; i < 6; i++) {
        int dr = (int)r - k_palette_rgb[i][0];
        int dg = (int)g - k_palette_rgb[i][1];
        int db = (int)b - k_palette_rgb[i][2];
        int d = dr * dr + dg * dg + db * db;
        if (d < best_d) {
            best_d = d;
            best_i = i;
        }
    }
    return k_palette_idx[best_i];
}

static size_t bmp_src_read(bmp_src_t *src, void *dst, size_t n)
{
    if (src->f) {
        return fread(dst, 1, n, src->f);
    }
    if (src->mem_pos >= src->mem_size) {
        return 0;
    }
    if ((src->mem_pos + n) > src->mem_size) {
        n = src->mem_size - src->mem_pos;
    }
    memcpy(dst, src->mem + src->mem_pos, n);
    src->mem_pos += n;
    return n;
}

static int bmp_src_seek(bmp_src_t *src, uint32_t off)
{
    if (src->f) {
        return fseek(src->f, (long)off, SEEK_SET);
    }
    if (off > src->mem_size) {
        return -1;
    }
    src->mem_pos = off;
    return 0;
}

UBYTE GUI_ReadBmp_RGB_6Color(const char *path, UWORD Xstart, UWORD Ystart)
{
    BMPFILEHEADER bmpFileHeader;  //Define a bmp file header structure
    BMPINFOHEADER bmpInfoHeader;  //Define a bmp info header structure
    bmp_src_t src = {0};

    src.f = fopen(path, "rb");
    if (src.f != NULL) {
        ESP_LOGI(TAG, "open the file done!");
    } else if (strcmp(bmp_basename(path), "peacock.bmp") == 0) {
        src.mem = peacock_bmp_start;
        src.mem_size = (size_t)(peacock_bmp_end - peacock_bmp_start);
        ESP_LOGI(TAG, "open embedded peacock.bmp, %u bytes", (unsigned)src.mem_size);
    } else {
        ESP_LOGE(TAG, "Cann't open the file!");
        return 0;
    }

    uint32_t br = (uint32_t)bmp_src_read(&src, &bmpFileHeader, sizeof(BMPFILEHEADER));
    if(br != sizeof(BMPFILEHEADER))
    {
        ESP_LOGE(TAG,"Failed to read BMP file header");
        if (src.f) {
            fclose(src.f);
        }
        return 0;
    }
    br = (uint32_t)bmp_src_read(&src, &bmpInfoHeader, sizeof(BMPINFOHEADER));
    if(br != sizeof(BMPINFOHEADER))
    {
        ESP_LOGE(TAG,"BmpInfoHeader error");
        if (src.f) {
            fclose(src.f);
        }
        return 0;
    }
    if((bmpInfoHeader.biWidth == 1200)&&(bmpInfoHeader.biHeight == 1600))
        Paint_SetRotate(0);
    else if((bmpInfoHeader.biWidth == 1600)&&(bmpInfoHeader.biHeight == 1200))
        Paint_SetRotate(90);
    ESP_LOGI(TAG,"pixel = %ld x %ld",bmpInfoHeader.biWidth,bmpInfoHeader.biHeight);
    
    int bits_per_pixel = bmpInfoHeader.biBitCount;
    if ((bits_per_pixel != 24) && (bits_per_pixel != 8)) {
        ESP_LOGE(TAG, "Unsupported BMP depth: %d bpp", bits_per_pixel);
        if (src.f) {
            fclose(src.f);
        }
        return 0;
    }

    UBYTE palette_colors[256] = {0};
    if (bits_per_pixel == 8) {
        uint32_t palette_count = bmpInfoHeader.biClrUsed;
        if ((palette_count == 0) || (palette_count > 256)) {
            palette_count = 256;
        }
        for (uint32_t i = 0; i < palette_count; i++) {
            BMPRGBQUAD entry;
            if (bmp_src_read(&src, &entry, sizeof(entry)) != sizeof(entry)) {
                ESP_LOGE(TAG, "Failed to read BMP palette");
                if (src.f) {
                    fclose(src.f);
                }
                return 0;
            }
            palette_colors[i] = rgb_to_epd6(entry.rgbRed, entry.rgbGreen, entry.rgbBlue);
        }
    }

    uint32_t row_stride =
        (((uint32_t)bmpInfoHeader.biWidth * (uint32_t)bits_per_pixel + 31U) / 32U) * 4U;

    // Read image data into the paint buffer.
    UWORD x, y;
    UBYTE Rdata[3];

    for(y = 0; y < bmpInfoHeader.biHeight; y++) {//Total display column
        if (bmp_src_seek(&src, bmpFileHeader.bOffset + (uint32_t)y * row_stride) != 0) {
            ESP_LOGE(TAG, "Failed to seek BMP row");
            break;
        }
        for(x = 0; x < bmpInfoHeader.biWidth ; x++) {//Show a line in the line
            UBYTE color;
            if (bits_per_pixel == 24) {
                if (bmp_src_read(&src, Rdata, 3) != 3) {
                    break;
                }
                color = rgb_to_epd6(Rdata[2], Rdata[1], Rdata[0]);
            } else {
                UBYTE palette_index;
                if (bmp_src_read(&src, &palette_index, 1) != 1) {
                    break;
                }
                color = palette_colors[palette_index];
            }

            Paint_SetPixel(Xstart+x, Ystart+bmpInfoHeader.biHeight-1-y, color);
            // Paint_SetPixel(Xstart + bmpInfoHeader.biWidth-1-x, Ystart + y, color);
        }
        // When reading a large amount of data, increase the delay to facilitate the CPU in processing the content of other threads
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (src.f) {
        fclose(src.f);
    }
    return 0;
}

