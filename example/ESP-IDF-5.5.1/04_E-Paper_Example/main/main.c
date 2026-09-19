#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "GUI_Paint.h"
#include "epaper_port.h"
#include "esp_log.h"
#include "jpeg_paint.h"

uint8_t *Image_Mono = NULL;
static const char *TAG = "main";

size_t buffer_size = EPD_WIDTH * EPD_HEIGHT / 2;

#define DECLARE_PDF_PAGE(number) \
    extern const uint8_t pdf_page_##number##_start[] \
        asm("_binary_page_" #number "_jpg_start"); \
    extern const uint8_t pdf_page_##number##_end[] \
        asm("_binary_page_" #number "_jpg_end")

DECLARE_PDF_PAGE(01);
DECLARE_PDF_PAGE(02);
DECLARE_PDF_PAGE(03);
DECLARE_PDF_PAGE(04);
DECLARE_PDF_PAGE(05);
DECLARE_PDF_PAGE(06);
DECLARE_PDF_PAGE(07);
DECLARE_PDF_PAGE(08);
DECLARE_PDF_PAGE(09);
DECLARE_PDF_PAGE(10);
DECLARE_PDF_PAGE(11);
DECLARE_PDF_PAGE(12);
DECLARE_PDF_PAGE(13);
DECLARE_PDF_PAGE(14);
DECLARE_PDF_PAGE(15);

#undef DECLARE_PDF_PAGE

typedef struct {
    const uint8_t *start;
    const uint8_t *end;
} embedded_page_t;

static const embedded_page_t s_thingboot_pages[] = {
    {pdf_page_01_start, pdf_page_01_end},
    {pdf_page_02_start, pdf_page_02_end},
    {pdf_page_03_start, pdf_page_03_end},
    {pdf_page_04_start, pdf_page_04_end},
    {pdf_page_05_start, pdf_page_05_end},
    {pdf_page_06_start, pdf_page_06_end},
    {pdf_page_07_start, pdf_page_07_end},
    {pdf_page_08_start, pdf_page_08_end},
    {pdf_page_09_start, pdf_page_09_end},
    {pdf_page_10_start, pdf_page_10_end},
    {pdf_page_11_start, pdf_page_11_end},
    {pdf_page_12_start, pdf_page_12_end},
    {pdf_page_13_start, pdf_page_13_end},
    {pdf_page_14_start, pdf_page_14_end},
    {pdf_page_15_start, pdf_page_15_end},
};

void app_main(void)
{
    ESP_LOGI(TAG, "1.e-Paper Init and Clear...");
    EPD_Port_Init();
    EPD_Init();
    EPD_Clear(EPD_WHITE);
    vTaskDelay(pdMS_TO_TICKS(2000));

    if ((Image_Mono = (uint8_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM)) == NULL) {
        ESP_LOGE(TAG, "Failed to apply for black memory...");
        return;
    }
    ESP_LOGI(TAG, "OK to apply for black memory, memory size: %d", buffer_size);
    Paint_NewImage(Image_Mono, EPD_WIDTH, EPD_HEIGHT, 0, EPD_WHITE);
    Paint_SetScale(6);
    Paint_SelectImage(Image_Mono);


#if 0
    // Optional GUI_Paint primitives demo.
    Paint_NewImage(Image_Mono, EPD_WIDTH, EPD_HEIGHT, 0, EPD_WHITE);
    Paint_SetScale(6);
    printf("SelectImage:Image\r\n");
    Paint_SelectImage(Image_Mono);
    Paint_Clear(EPD_BLUE);
 

    Paint_DrawPoint(10, 80, EPD_RED, DOT_PIXEL_1X1, DOT_STYLE_DFT);
    Paint_DrawPoint(10, 90, EPD_BLUE, DOT_PIXEL_2X2, DOT_STYLE_DFT);
    Paint_DrawPoint(10, 100, EPD_GREEN, DOT_PIXEL_3X3, DOT_STYLE_DFT);
    Paint_DrawLine(20, 70, 70, 120, EPD_YELLOW, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(70, 70, 20, 120, EPD_YELLOW, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawRectangle(20, 70, 70, 120, EPD_BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(80, 70, 130, 120, EPD_BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawCircle(45, 95, 20, EPD_BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(105, 95, 20, EPD_WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawLine(85, 95, 125, 95, EPD_YELLOW, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    Paint_DrawLine(105, 75, 105, 115, EPD_YELLOW, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);

    Paint_Clear(EPD_BLACK);
    Paint_DrawString_CN(10, 130, "你好abc", &Font16_UTF8, EPD_BLACK, EPD_WHITE);
    Paint_DrawString_CN(10, 170, "微雪电子", &Font16_UTF8, EPD_WHITE, EPD_BLACK);
    Paint_DrawNum(10, 10, 123456789, &Font12, EPD_BLACK, EPD_WHITE);
    Paint_DrawNum(10, 40, 987654321, &Font12, EPD_WHITE, EPD_BLACK);
    Paint_DrawString_EN(145, 0, "Waveshare", &Font16, EPD_BLACK, EPD_WHITE);
    Paint_DrawString_EN(145, 35, "Waveshare", &Font16, EPD_GREEN, EPD_WHITE);
    Paint_DrawString_EN(145, 70, "Waveshare", &Font16, EPD_BLUE, EPD_WHITE);
    Paint_DrawString_EN(145, 105, "Waveshare", &Font16, EPD_RED, EPD_WHITE);
    Paint_DrawString_EN(145, 140, "Waveshare", &Font16, EPD_YELLOW, EPD_WHITE);

    Paint_DrawString_EN(200, 200, "Pa-Tech", &Font24, EPD_BLACK, EPD_WHITE);
    Paint_DrawString_EN(200, 235, "Pa-Tech", &Font24, EPD_GREEN, EPD_WHITE);
    Paint_DrawString_EN(200, 270, "Waveshare", &Font16, EPD_BLUE, EPD_WHITE);
    Paint_DrawString_EN(200, 305, "Waveshare", &Font16, EPD_RED, EPD_WHITE);
    Paint_DrawString_EN(200, 340, "Waveshare", &Font16, EPD_YELLOW, EPD_RED);

    printf("EPD_Display\r\n");
    EPD_Display(Image_Mono);
    vTaskDelay(pdMS_TO_TICKS(3000));
#endif

#if 1
    while (1) {
        for (size_t page = 0;
             page < sizeof(s_thingboot_pages) / sizeof(s_thingboot_pages[0]);
             page++) {
            Paint_Clear(EPD_WHITE);
            ESP_LOGI(TAG, "Paint ThingBoot page %u of 15", (unsigned)(page + 1));

            const embedded_page_t *image = &s_thingboot_pages[page];
            if (Paint_DrawJpeg6Color(image->start,
                                    (size_t)(image->end - image->start))) {
                EPD_Display(Image_Mono);
                vTaskDelay(pdMS_TO_TICKS(10000));
            }
        }
    }
#endif

    ESP_LOGI(TAG, "clear display; EPD sleep is disabled");

    EPD_Clear(EPD_WHITE);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // EPD_Sleep() intentionally disabled.
}
