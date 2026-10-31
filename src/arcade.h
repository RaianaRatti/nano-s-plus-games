// Badge Arcade: the games of BadgeOS on a Ledger Nano S Plus.
//
// The pieces:
//   gfx.c      a 128 x 64, 1 bit framebuffer shown through one full-screen NBGL image
//   ui.c       the receipt look of BadgeOS, shrunk to the Nano screen: header, rules, footer
//   input.c    two buttons turned into gestures: LEFT, RIGHT, BOTH (select) and HOLD (back)
//   shell.c    the launcher and the switch between apps
//   main.c     the event loop: every event goes to the OS first, then to the arcade
//   run.c      the life of a score game: title, play, pause, crash, game over, best score
//   dodge.c, dice.c, duel.c   the games of BadgeOS
//   snake.c, pong.c, bricks.c, racer.c, cave.c, flip.c, runner.c   the arcade's own games
//
// Time is counted in ticks. The OS sends a ticker event every 100 ms, so the games update at
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

// ---------------------------------------------------------------------------------------------
// The receipt look: ui.c
// ---------------------------------------------------------------------------------------------

#define UI_HEADER  "BADGEOS"
#define UI_TOP     10  // the first row under the header's rule
#define UI_BOTTOM  54  // the footer's rule

void ui_dirty(void);
bool ui_take_dirty(void);
void ui_page(void);
void ui_header(const char *left, const char *right);
void ui_footer(const char *left, const char *right);
void ui_rule(int y);

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
// Shell: shell.c
// ---------------------------------------------------------------------------------------------

typedef enum {
    APP_LAUNCHER,
    APP_DODGE,
    APP_DICE,
    APP_DUEL,
    APP_SNAKE,
    APP_PONG,
    APP_BRICKS,
    APP_RACER,
    APP_CAVE,
    APP_FLIP,
    APP_RUNNER,
} app_id_t;

// The launcher shows one card per game, in the order above, then Quit.
#define NB_GAMES (APP_RUNNER - APP_DODGE + 1)

void shell_init(void);
void shell_open(app_id_t app);
void shell_back(void);
void shell_event(event_t event, uint8_t buttons);
void shell_tick(void);
void shell_suspend(void);
void shell_render(void);

// ---------------------------------------------------------------------------------------------
// Persistent scores: storage.c
// ---------------------------------------------------------------------------------------------

// One best score per game that keeps one.
typedef enum {
    BEST_DODGE,
    BEST_SNAKE,
    BEST_BRICKS,
    BEST_RACER,
    BEST_CAVE,
    BEST_FLIP,
    BEST_RUNNER,
    NB_BEST,
} best_t;

uint32_t storage_best(best_t slot);
void     storage_set_best(best_t slot, uint32_t score);

// ---------------------------------------------------------------------------------------------
// Random numbers from the secure element's hardware generator: random.c
// ---------------------------------------------------------------------------------------------

// A uniform number in [low, high).
uint32_t random_range(uint32_t low, uint32_t high);

// ---------------------------------------------------------------------------------------------
// A run of a score game: run.c
// ---------------------------------------------------------------------------------------------

typedef enum { RUN_TITLE, RUN_PLAY, RUN_PAUSED, RUN_CRASH, RUN_OVER } run_mode_t;

typedef struct {
    run_mode_t mode;
    best_t     slot;
    uint8_t    ticks;  // left in the crash, or before the game-over screen returns to the title
    bool       new_best;
    uint32_t   score;
    uint32_t   best;
} run_t;

typedef enum {
    RUN_NOTHING,  // the run used the event
    RUN_START,    // a new run begins: set up the world
    RUN_INPUT,    // in play: the event is for the game
} run_action_t;

void         run_enter(run_t *run, best_t slot);
run_action_t run_event(run_t *run, event_t event);
bool         run_tick(run_t *run);  // true when the world should move
void         run_suspend(run_t *run);
void         run_crash(run_t *run);  // still field for a moment, then game over
void         run_over(run_t *run);   // game over at once
void         run_draw_title(const run_t *run, const char *name, const char *hint, const char *unit);
void         run_draw_paused(void);
void         run_draw_over(const run_t *run, const char *name, const char *label, const char *unit);

// ---------------------------------------------------------------------------------------------
// The apps
// ---------------------------------------------------------------------------------------------

void launcher_enter(void);
void launcher_event(event_t event, uint8_t buttons);
void launcher_draw(void);

void dodge_enter(void);
void dodge_event(event_t event, uint8_t buttons);
void dodge_tick(void);
void dodge_suspend(void);
void dodge_draw(void);

void dice_enter(void);
void dice_event(event_t event, uint8_t buttons);
void dice_tick(void);
void dice_draw(void);

void duel_enter(void);
void duel_event(event_t event, uint8_t buttons);
void duel_tick(void);
void duel_draw(void);

// Each of these games has the same five entry points.
#define GAME_ENTRY_POINTS(name)                  \
    void name##_enter(void);                     \
    void name##_event(event_t event, uint8_t b); \
    void name##_tick(void);                      \
    void name##_suspend(void);                   \
    void name##_draw(void);

GAME_ENTRY_POINTS(snake)
GAME_ENTRY_POINTS(pong)
GAME_ENTRY_POINTS(bricks)
GAME_ENTRY_POINTS(racer)
GAME_ENTRY_POINTS(cave)
GAME_ENTRY_POINTS(flip)
GAME_ENTRY_POINTS(runner)
