// Two buttons, five events.
//
// BadgeOS had UP, DOWN, LEFT, RIGHT, SELECT and CANCEL. The Nano has LEFT and RIGHT, and the
// Ledger way to get more out of them is to press both:
//
//   EV_DOWN     the instant a button goes down (the games use it: Duel, the first step in Dodge)
//   EV_LEFT     LEFT pressed and released alone        (previous / less)
//   EV_RIGHT    RIGHT pressed and released alone       (next / more)
//   EV_SELECT   both pressed and released quickly      (BadgeOS's SELECT)
//   EV_BACK     both held for HOLD_TICKS               (BadgeOS's CANCEL)
//
// A gesture lasts from the first button down to the last button up, so pressing both never also
// counts as a LEFT or a RIGHT.
//
// The device's MCU sends the state of both buttons on each change (and repeats it while a button
// is held): 1, 3, 1, 0 for "left down, right down, right up, left up". Speculos, the emulator,
// sends only the button that went down, and 0 when any goes up: 1, 2, 0, 0. NBGL copes with both
// by adding the buttons up until a 0; the arcade does the same for a new button, but still follows
// the device when one of two is released, because a game must know which one is still held.

#include "arcade.h"

static uint8_t mask;      // the buttons down now
static uint8_t gesture;   // every button that went down since all were up
static uint8_t hold;      // ticks for which both have been down
static bool    consumed;  // the gesture already produced its event (or was swallowed)

void input_reset(void)
{
    mask     = 0;
    gesture  = 0;
    hold     = 0;
    consumed = false;
}

// Ignore the buttons that are down now until they are released: a screen that has just appeared
// must not act on the end of a press meant for the screen before it.
void input_swallow(void)
{
    if (mask) {
        consumed = true;
    }
}

uint8_t input_held(void)
{
    return mask;
}

void input_buttons(uint8_t state)
{
    state &= BTN_BOTH;
    uint8_t now;
    if (state == 0 || (state & mask) != 0) {
        now = state;         // the whole state: all up, the same, one more, or one of two released
    }
    else {
        now = mask | state;  // only the button that went down (the emulator): add it
    }
    uint8_t pressed = now & ~mask;
    mask            = now;
    gesture |= now;

    if (pressed) {
        shell_event(EV_DOWN, pressed);
    }

    if (now == 0 && gesture) {
        bool    done = consumed;
        uint8_t was  = gesture;
        gesture      = 0;
        hold         = 0;
        consumed     = false;
        if (!done) {
            if (was == BTN_BOTH) {
                shell_event(EV_SELECT, was);
            }
            else if (was == BTN_LEFT) {
                shell_event(EV_LEFT, was);
            }
            else {
                shell_event(EV_RIGHT, was);
            }
        }
    }
}

void input_tick(void)
{
    if (mask != BTN_BOTH) {
        hold = 0;
        return;
    }
    if (!consumed && ++hold >= HOLD_TICKS) {
        consumed = true;
        shell_event(EV_BACK, BTN_BOTH);
    }
}
