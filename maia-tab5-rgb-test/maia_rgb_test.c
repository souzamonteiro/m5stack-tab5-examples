/* MaiaRTOS Tab5 LCD RGB diagnostic - freestanding C99, no Arduino/FreeRTOS.
 * This file draws into a framebuffer supplied by the existing MaiaRTOS BSP.
 * It does NOT initialize ESP32-P4 MIPI-DSI hardware.
 */
#include <stdint.h>
#include <stddef.h>
#include "maia_tab5_display_port.h"

static void fill_rgb565(uint8_t *base, uint32_t width, uint32_t height,
                        uint32_t stride_bytes, int bgr_order)
{
    uint32_t x, y;
    for (y = 0; y < height; ++y) {
        uint16_t color;
        uint8_t *row = base + (size_t)y * stride_bytes;
        if (y < height / 3u) color = bgr_order ? 0x001fu : 0xf800u;
        else if (y < (height * 2u) / 3u) color = 0x07e0u;
        else color = bgr_order ? 0xf800u : 0x001fu;
        for (x = 0; x < width; ++x) {
            /* Explicit little-endian bytes, independent of host CPU alignment. */
            row[2u*x] = (uint8_t)(color & 0xffu);
            row[2u*x+1u] = (uint8_t)(color >> 8);
        }
    }
}

static void fill_rgb888(uint8_t *base, uint32_t width, uint32_t height,
                        uint32_t stride_bytes, int bgr_order)
{
    uint32_t x, y;
    for (y = 0; y < height; ++y) {
        uint8_t r = y < height / 3u ? 255u : 0u;
        uint8_t g = (y >= height / 3u && y < (height * 2u) / 3u) ? 255u : 0u;
        uint8_t b = y >= (height * 2u) / 3u ? 255u : 0u;
        uint8_t *row = base + (size_t)y * stride_bytes;
        for (x = 0; x < width; ++x) {
            row[3u*x] = bgr_order ? b : r;
            row[3u*x+1u] = g;
            row[3u*x+2u] = bgr_order ? r : b;
        }
    }
}

static void fill_xrgb8888(uint8_t *base, uint32_t width, uint32_t height,
                          uint32_t stride_bytes, int bgr_order)
{
    uint32_t x, y;
    for (y = 0; y < height; ++y) {
        uint8_t r = y < height / 3u ? 255u : 0u;
        uint8_t g = (y >= height / 3u && y < (height * 2u) / 3u) ? 255u : 0u;
        uint8_t b = y >= (height * 2u) / 3u ? 255u : 0u;
        uint8_t *row = base + (size_t)y * stride_bytes;
        for (x = 0; x < width; ++x) {
            /* Memory byte order: B,G,R,X for XRGB8888 LE; R,G,B,X for XBGR8888 LE. */
            row[4u*x] = bgr_order ? r : b;
            row[4u*x+1u] = g;
            row[4u*x+2u] = bgr_order ? b : r;
            row[4u*x+3u] = 0xffu;
        }
    }
}

int maia_rgb_test_run(void)
{
    maia_display_surface_t s;
    uint32_t bytes_per_pixel;
    size_t needed;
    if (maia_display_open(&s) != 0) return -1;
    if (!s.pixels || !s.width || !s.height) return -2;
    if (s.format == MAIA_PIXEL_RGB565_LE) bytes_per_pixel = 2u;
    else if (s.format == MAIA_PIXEL_RGB888 || s.format == MAIA_PIXEL_BGR888) bytes_per_pixel = 3u;
    else if (s.format == MAIA_PIXEL_XRGB8888_LE || s.format == MAIA_PIXEL_XBGR8888_LE) bytes_per_pixel = 4u;
    else return -3;
    if (s.width > UINT32_MAX / bytes_per_pixel ||
        s.stride_bytes < s.width * bytes_per_pixel) return -4;
    if (s.height > SIZE_MAX / s.stride_bytes) return -5;
    needed = (size_t)s.height * s.stride_bytes;
    if (needed > s.capacity_bytes) return -6;
    if (s.format == MAIA_PIXEL_RGB565_LE)
        fill_rgb565(s.pixels, s.width, s.height, s.stride_bytes, 0);
    else if (s.format == MAIA_PIXEL_RGB888 || s.format == MAIA_PIXEL_BGR888)
        fill_rgb888(s.pixels, s.width, s.height, s.stride_bytes, s.format == MAIA_PIXEL_BGR888);
    else
        fill_xrgb8888(s.pixels, s.width, s.height, s.stride_bytes, s.format == MAIA_PIXEL_XBGR8888_LE);
    /* BSP must handle cache flush / DMA sync and scanout or transfer. */
    return maia_display_present(&s);
}

#ifdef MAIA_RGB_TEST_STANDALONE
int main(void) { return maia_rgb_test_run(); }
#endif
