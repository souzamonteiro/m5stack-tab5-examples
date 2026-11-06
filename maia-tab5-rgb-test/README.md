# MaiaRTOS / M5Stack Tab5: RGB LCD diagnostic

This is a **real, compilable C99 framebuffer drawing test**, not a complete ESP32-P4 MIPI-DSI driver. It does not use Arduino, FreeRTOS, LVGL, or ESP-IDF APIs. It cannot run on the Tab5 until the two `maia_display_*` functions are implemented using the **existing, audited MaiaRTOS BSP**.

## Expected result

Three static horizontal bands: red (top), green (middle), blue (bottom). No shell or keyboard involved.

## Files

- `maia_rgb_test.c`: draws the RGB test image and checks framebuffer bounds.
- `maia_tab5_display_port.h`: strict integration contract.
- `maia_tab5_display_port_example.c`: explanatory template, intentionally not a driver.

## Integrating

Implement in the Tab5 BSP:

```c
int maia_display_open(maia_display_surface_t *out);
int maia_display_present(const maia_display_surface_t *surface);
```

`open()` must initialize the panel, configure MIPI-DSI and backlight, and return the **real** writable framebuffer with accurate dimensions, stride, capacity, and pixel format. `present()` must perform cache maintenance and DMA/DSI synchronization as needed, and submit the frame if the display is not continuously scanning the framebuffer.

Call `maia_rgb_test_run()` **after** essential bare-metal startup and before launching the shell. Do not assume that `app_main()` is bare-metal: in ESP-IDF it normally runs inside a FreeRTOS task.

Do not use `printf()` as evidence of LCD output. Serial and LCD are independent output paths.

## Interpretation

- Correct three bands: pixel write, format, stride, and scanout path are probably functioning.
- Solid cyan: check pixel format/order, framebuffer ownership, cache, scanout setup, and panel initialization.
- Vertical bars: check stride/width/format, DMA descriptors, DSI timing and panel-specific init.
- Black display: check power, reset, backlight, DSI link, framebuffer address.

**Important:** ESP32-P4 Tab5 panel controller/revision must be identified from the actual device/BSP. Do not copy random MIPI-DSI initialization tables.

## Compile syntax check

```sh
cc -std=c99 -Wall -Wextra -Werror -pedantic -fsyntax-only maia_rgb_test.c
```

This checks the drawing code only, not hardware integration or bootability.
