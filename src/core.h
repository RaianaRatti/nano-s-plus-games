// The core the apps are built on (Badge Arcade, the pet, the gallery): the screen, the look, the two
// buttons, the random generator and the event loop. Nothing in here knows a game.
//
//   gfx.c      a 128 x 64, 1 bit framebuffer shown through one full-screen NBGL image
//   ui.c       the receipt look of BadgeOS, shrunk to the Nano screen: header, rules, footer
//   input.c    two buttons turned into gestures: LEFT, RIGHT, BOTH (select) and HOLD (back)
//   random.c   the secure element's hardware random generator
//   main.c     the event loop: every event goes to the OS first, then to the app's shell
//
// Each app provides the shell below, which the event loop and the input call.
//
// Time is counted in ticks. The OS sends a ticker event every 100 ms, so the apps update at
// 10 Hz and move on a button the moment it is pressed.

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TICK_MS 100

// ---------------------------------------------------------------------------------------------
// Display: gfx.c
// ---------------------------------------------------------------------------------------------

#define GFX_W 128
#define GFX_H 64

#define FONT_W   5  // a glyph is 5 x 7 pixels ...
#define FONT_H   7
#define FONT_ADV 6  // ... and the next one starts 6 pixels to the right (times the scale)

// Glyphs that are not ASCII.
#define G_LEFT  "\x01"  // a triangle pointing left
#define G_RIGHT "\x02"  // a triangle pointing right
#define G_DOT   "\x03"  // a middle dot
#define G_HEART "\x04"
#define G_STAR  "\x05"
#define G_E_ACUTE "\x06"  // é and á, for names
#define G_A_ACUTE "\x07"

void gfx_init(void);
void gfx_present(void);
void gfx_clear(bool on);
void gfx_pixel(int x, int y, bool on);
void gfx_fill_rect(int x, int y, int w, int h, bool on);
void gfx_rect(int x, int y, int w, int h, bool on);
void gfx_round_rect(int x, int y, int w, int h, bool on);
void gfx_dotted_hline(int x, int y, int w, bool on);
int  gfx_text_width(const char *text, int scale);
int  gfx_text(int x, int y, const char *text, int scale, bool on);
void gfx_text_center(int cx, int y, const char *text, int scale, bool on);
void gfx_text_right(int right, int y, const char *text, int scale, bool on);

// Draws a 1 bit picture whose rows are `w` bits wide, the leftmost pixel in the highest of them:
// 0b0110 is ".XX.". Only the set bits are drawn (lit, or cut out when on is false). flip draws it
// upside down.
void gfx_sprite(int x, int y, const uint16_t *rows, int w, int h, bool flip, bool on);
void gfx_sprite_scaled(int x, int y, const uint16_t *rows, int w, int h, bool flip, bool on, int scale);
void gfx_bubble(int x, int y, int w, int h, int r, bool on);

// ---------------------------------------------------------------------------------------------
// The look: ui.c
// ---------------------------------------------------------------------------------------------

#define UI_TOP 10  // the first row under the bar

void ui_dirty(void);
bool ui_take_dirty(void);
void ui_page(void);
void ui_bar(const char *left, const char *right);
void ui_rule(int y);
void ui_bubble_text(int cx, int y, const char *text, int scale);
void ui_best(int cx, int y, uint32_t best);
void ui_pause_sign(void);

// Formats an unsigned number, optionally followed by a suffix, into out (at least 16 bytes).
void ui_number(char *out, uint32_t value, const char *suffix);

// ---------------------------------------------------------------------------------------------
// Input: input.c
// ---------------------------------------------------------------------------------------------

#define BTN_LEFT  0x01
#define BTN_RIGHT 0x02
#define BTN_BOTH  (BTN_LEFT | BTN_RIGHT)

typedef enum {
    EV_DOWN,    // buttons went down (the bits in `buttons`): for games that need the instant
    EV_LEFT,    // the left button was clicked alone (on release)
    EV_RIGHT,   // the right button was clicked alone (on release)
    EV_SELECT,  // both buttons were pressed together and released
    EV_BACK,    // both buttons were held for HOLD_TICKS
} event_t;

#define HOLD_TICKS 8  // 0.8 s

void    input_reset(void);
void    input_buttons(uint8_t mask);
void    input_tick(void);
void    input_swallow(void);
uint8_t input_held(void);

// ---------------------------------------------------------------------------------------------
// Random numbers from the secure element's hardware generator: random.c
// ---------------------------------------------------------------------------------------------

// A uniform number in [low, high).
uint32_t random_range(uint32_t low, uint32_t high);

// ---------------------------------------------------------------------------------------------
// The app's shell: provided by each app
// ---------------------------------------------------------------------------------------------

void shell_init(void);                             // the app opens
void shell_event(event_t event, uint8_t buttons);  // a gesture (from input.c)
void shell_tick(void);                             // every TICK_MS
void shell_suspend(void);                          // the OS took the screen (the device locked)
void shell_render(void);                           // draw the screen if it changed
void shell_back(void);                             // HOLD from the top: back one level, or out
