// The event loop.
//
// A Nano app runs inside the event loop of the SDK: the app waits for USB commands with
// io_recv_command(), and while it waits the SDK calls io_event() for each event the MCU sends
// (a button changed, 100 ms passed, ...). This file replaces the SDK's default io_event() so that
// the arcade sees the raw state of the buttons, which a game needs (is LEFT held down right now?),
// instead of only NBGL's menu gestures.
//
// Every event goes to the OS first, exactly as the SDK's own handler does it (lib_ux_nbgl/ux.c).
// When the OS is showing its own screen (the lock screen, the PIN pad) it keeps the event, the
// arcade draws nothing and the running game is paused. When the OS gives the screen back it asks
// for a redraw and the last frame is shown again.

#include "os.h"
#include "io.h"
#include "ux.h"
#include "nbgl_obj.h"
#include "nbgl_screen.h"

#include "arcade.h"

// The status word for "instruction not supported": the arcade answers no USB command.
#define SW_INS_NOT_SUPPORTED 0x6D00

// Gives the event to the OS. Returns true when the app may use it and draw.
static bool forward_to_os(void)
{
    G_ux_params.ux_id = BOLOS_UX_EVENT;
    G_ux_params.len   = 0;
    os_ux(&G_ux_params);
    G_ux_params.len = os_sched_last_status(TASK_BOLOS_UX);
    if (G_ux_params.len == BOLOS_UX_REDRAW) {
        // The OS has just given the screen back: show the last frame again.
        nbgl_objAllowDrawing(true);
        nbgl_screenRedraw();
        nbgl_refresh();
        return false;
    }
    return G_ux_params.len != BOLOS_UX_IGNORE && G_ux_params.len != BOLOS_UX_CONTINUE;
}

// The OS kept the event: whatever the buttons were doing belongs to the OS screen now.
static void os_has_screen(void)
{
    input_reset();
    shell_suspend();
}

unsigned char io_event(unsigned char channel)
{
    UNUSED(channel);
    bool allowed;

    switch (G_io_seproxyhal_spi_buffer[0]) {
        case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
            allowed = forward_to_os();
            nbgl_objAllowDrawing(allowed);
            if (allowed) {
                // Bit 0 is the left button, bit 1 the right one.
                input_buttons(G_io_seproxyhal_spi_buffer[3] >> 1);
                shell_render();
            }
            else {
                os_has_screen();
            }
            nbgl_refresh();
            break;

        case SEPROXYHAL_TAG_TICKER_EVENT:
            allowed = forward_to_os();
            nbgl_objAllowDrawing(allowed);
            if (allowed) {
                nbgl_screenHandler(TICK_MS);
                input_tick();
                shell_tick();
                shell_render();
            }
            else {
                os_has_screen();
            }
            nbgl_refresh();
            break;

        default:
            UX_DEFAULT_EVENT();
            break;
    }
    return 1;
}

void app_main(void)
{
    io_init();
    gfx_init();
    shell_init();
    shell_render();
    nbgl_refresh();

    // Nothing to do over USB: refuse every command. The games run inside the events above.
    for (;;) {
        if (io_recv_command() < 0) {
            return;
        }
        io_send_sw(SW_INS_NOT_SUPPORTED);
    }
}
