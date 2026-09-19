#include "jpeg_paint.h"

#include <limits.h>
#include <string.h>

#include "esp32s3/rom/tjpgd.h"
#include "esp_log.h"
#include "GUI_Paint.h"
#include "epaper_port.h"

typedef struct {
    const uint8_t *data;
    size_t size;
    size_t position;
    UWORD x_offset;
    UWORD y_offset;
} jpeg_context_t;

static const char *TAG = "jpeg_paint";
static uint8_t s_jpeg_work[4096];

static const uint8_t s_palette_rgb[6][3] = {
    {  0,   0,   0},
    {255, 255, 255},
    {255, 255,   0},
    {255,   0,   0},
    {  0,   0, 255},
    {  0, 255,   0},
};
static const uint8_t s_palette_index[6] = {0, 1, 2, 3, 5, 6};

static uint8_t rgb_to_epd6(uint8_t red, uint8_t green, uint8_t blue)
{
    int best = 0;
    int best_distance = INT_MAX;

    for (int i = 0; i < 6; i++) {
        int dr = (int)red - s_palette_rgb[i][0];
        int dg = (int)green - s_palette_rgb[i][1];
        int db = (int)blue - s_palette_rgb[i][2];
        int distance = dr * dr + dg * dg + db * db;
        if (distance < best_distance) {
            best_distance = distance;
            best = i;
        }
    }
    return s_palette_index[best];
}

static UINT jpeg_input(JDEC *decoder, BYTE *buffer, UINT bytes)
{
    jpeg_context_t *context = (jpeg_context_t *)decoder->device;
    size_t available = context->size - context->position;
    if (bytes > available) {
        bytes = (UINT)available;
    }
    if (buffer != NULL) {
        memcpy(buffer, context->data + context->position, bytes);
    }
    context->position += bytes;
    return bytes;
}

static UINT jpeg_output(JDEC *decoder, void *bitmap, JRECT *rectangle)
{
    jpeg_context_t *context = (jpeg_context_t *)decoder->device;
    const uint8_t *rgb = (const uint8_t *)bitmap;

    for (WORD y = rectangle->top; y <= rectangle->bottom; y++) {
        for (WORD x = rectangle->left; x <= rectangle->right; x++) {
            Paint_SetPixel(context->x_offset + x, context->y_offset + y,
                           rgb_to_epd6(rgb[0], rgb[1], rgb[2]));
            rgb += 3;
        }
    }
    return 1;
}

bool Paint_DrawJpeg6Color(const uint8_t *data, size_t size)
{
    JDEC decoder;
    jpeg_context_t context = {
        .data = data,
        .size = size,
    };

    JRESULT result =
        jd_prepare(&decoder, jpeg_input, s_jpeg_work, sizeof(s_jpeg_work), &context);
    if (result != JDR_OK) {
        ESP_LOGE(TAG, "JPEG prepare failed: %d", result);
        return false;
    }

    BYTE scale = 0;
    while ((scale < 3) &&
           (((decoder.width >> scale) > EPD_WIDTH) ||
            ((decoder.height >> scale) > EPD_HEIGHT))) {
        scale++;
    }

    UINT width = decoder.width >> scale;
    UINT height = decoder.height >> scale;
    context.x_offset = (EPD_WIDTH - width) / 2;
    context.y_offset = (EPD_HEIGHT - height) / 2;
    Paint_SetRotate(0);

    result = jd_decomp(&decoder, jpeg_output, scale);
    if (result != JDR_OK) {
        ESP_LOGE(TAG, "JPEG decode failed: %d", result);
        return false;
    }

    ESP_LOGI(TAG, "JPEG decoded: %ux%u", width, height);
    return true;
}
