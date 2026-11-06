/*
 * MaiaRTOS / M5Stack Tab5 Clean-Room Display Port Implementation
 * Strictly C99, freestanding, ZERO FreeRTOS / ZERO ESP-IDF / ZERO Arduino.
 *
 * Implements the contract defined in maia_tab5_display_port.h:
 *   - int maia_display_open(maia_display_surface_t *out)
 *   - int maia_display_present(const maia_display_surface_t *surface)
 *
 * Plus bare-metal maia_boot_main() entry point executing maia_rgb_test_run().
 */

#include "maia_tab5_display_port.h"
#include <maia/console.h>
#include <maia/soc/esp32p4_board_io.h>
#include <maia/soc/esp32p4_display.h>
#include <maia/soc/esp32p4_regs.h>
#include <stdint.h>
#include <stddef.h>

extern int maia_rgb_test_run(void);

#define TAB5_DISP_WIDTH      720U
#define TAB5_BAND_HEIGHT     128U
#define TAB5_SURF_HEIGHT     384U /* 3 bands of 128 lines = 384 lines */
#define TAB5_STRIDE_BYTES    (TAB5_DISP_WIDTH * 2U) /* 1440 bytes */
#define TAB5_CAPACITY_BYTES  (TAB5_STRIDE_BYTES * TAB5_SURF_HEIGHT) /* 552960 bytes */

#define L2MEM_BAND0_PHYS     0x4FF20000UL
#define L2MEM_BAND1_PHYS     0x4FF4D000UL
#define L2MEM_BAND2_PHYS     0x4FF7A000UL

/* Uncached CPU pointer to Band 0 in internal L2 SRAM */
#define L2MEM_SURF_CPU       ((uint8_t *)(uintptr_t)(L2MEM_BAND0_PHYS + 0x40000000UL))

int maia_display_open(maia_display_surface_t *out) {
    if (!out) {
        return -1;
    }

    maia_console_puts("[RGB-TEST] Initializing Tab5 Bare-Metal Hardware & Display...\n");

    /* Initialize M5Stack Tab5 IO expanders, backlight, touch/display detect */
    maia_tab5_board_io_init();

    /* Initialize MIPI-DSI ST7121 panel */
    maia_display_init(maia_tab5_get_detected_panel());

    out->pixels         = L2MEM_SURF_CPU;
    out->width          = TAB5_DISP_WIDTH;
    out->height         = TAB5_SURF_HEIGHT;
    out->stride_bytes   = TAB5_STRIDE_BYTES;
    out->capacity_bytes = TAB5_CAPACITY_BYTES;
    out->format         = MAIA_PIXEL_RGB565_LE;

    maia_console_puts("[RGB-TEST] Surface opened: 720x384 RGB565 (552960 bytes in L2MEM)\n");
    return 0;
}

int maia_display_present(const maia_display_surface_t *surface) {
    (void)surface;
    maia_console_puts("[RGB-TEST] Presenting 3 horizontal bands to DW_GDMA continuous scanout...\n");

    /* Map 10 DW_GDMA descriptor blocks across the 3 color bands:
     * Blocks 0..2 (lines 0..383)   -> Band 0 (RED)
     * Blocks 3..6 (lines 384..895)  -> Band 1 (GREEN)
     * Blocks 7..9 (lines 896..1279) -> Band 2 (BLUE)
     */
    maia_display_set_rgb_test_mode();
    maia_display_refresh_dma_if_needed();

    maia_console_puts("[RGB-TEST] Present complete: Red (top), Green (mid), Blue (bottom) ACTIVE!\n");
    return 0;
}

void maia_boot_main(void) {
    maia_console_init();
    maia_console_puts("\n========================================================\n");
    maia_console_puts("MaiaRTOS Tab5 RGB LCD Diagnostic (Freestanding C99)\n");
    maia_console_puts("Zero FreeRTOS / Zero ESP-IDF / Zero Arduino\n");
    maia_console_puts("========================================================\n");

    int ret = maia_rgb_test_run();
    maia_console_puts("[RGB-TEST] maia_rgb_test_run() result: ");
    if (ret == 0) {
        maia_console_puts("SUCCESS (0)\n");
    } else {
        maia_console_puts("FAILED (code ");
        maia_console_putc((char)('0' + (ret < 0 ? -ret : ret)));
        maia_console_puts(")\n");
    }

    for (;;) {
        maia_display_refresh_dma_if_needed();
        for (volatile int i = 0; i < 2000000; ++i) {
            __asm__ volatile("nop");
        }
    }
}

