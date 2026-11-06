/* Integration template ONLY. Not a functional Tab5 hardware driver.
 * Replace calls below with the real MaiaRTOS BSP APIs.
 * Do not compile this file until implemented. */
#include "maia_tab5_display_port.h"

#if defined(MAIA_TAB5_PORT_IMPLEMENTED)
#error "Replace this example with actual BSP integration before enabling it"
#endif

/* The BSP must implement:
 * 1. panel-specific power/reset and MIPI-DSI initialization
 * 2. known framebuffer address/size/format/stride
 * 3. any cache/DMA synchronization and frame presentation
 * 4. LCD backlight enable
 *
 * Do not use a guessed memory address, panel init table, or register value.
 */
