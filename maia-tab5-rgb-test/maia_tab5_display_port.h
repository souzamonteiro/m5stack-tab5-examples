#ifndef MAIA_TAB5_DISPLAY_PORT_H
#define MAIA_TAB5_DISPLAY_PORT_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MAIA_PIXEL_RGB565_LE = 1,
    MAIA_PIXEL_RGB888 = 2,
    MAIA_PIXEL_BGR888 = 3,
    MAIA_PIXEL_XRGB8888_LE = 4,
    MAIA_PIXEL_XBGR8888_LE = 5
} maia_pixel_format_t;

typedef struct {
    uint8_t *pixels;       /* CPU-accessible writable framebuffer */
    uint32_t width;
    uint32_t height;
    uint32_t stride_bytes; /* bytes per row, including any padding */
    size_t capacity_bytes; /* actual allocated framebuffer bytes */
    maia_pixel_format_t format;
} maia_display_surface_t;

/* Implement in existing Tab5 BSP. Must initialize panel, clocks, DSI and
 * backlight as appropriate, and return a valid writable framebuffer. */
int maia_display_open(maia_display_surface_t *out);
/* Flush caches / sync DMA / submit frame as required by real hardware. */
int maia_display_present(const maia_display_surface_t *surface);
#ifdef __cplusplus
}
#endif
#endif
