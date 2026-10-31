// Duel (BadgeOS apps/duel): a reaction duel, best of three.
//
// On BadgeOS two badges played over ESP-NOW and the loser paid a stake. The Nano has no radio and
// the arcade has no wallet, so the duel is played on one device by two people: the left player
// owns the left button, the right player the right one.
//
//   1. The screen says WAIT for a random 2 to 5 s (the inviter chose this delay on BadgeOS).
//   2. It flashes: the whole screen lights up.
//   3. The first to press wins the round. Pressing before the flash loses it. Nobody within 3 s,
//      or both at the same instant, is a tie and the round is played again.
//   4. The first to win two rounds wins the duel; after nine rounds it is a draw.
//
// Who was first is exact: it is the order of the button events. The reaction time shown is
// counted in ticks, so it is rounded to a tenth of a second.
//
//   Playing  the duel starts as it opens. HOLD (both players, together) leaves it.
//   Over     BOTH plays again, HOLD goes back to the launcher.

#include <string.h>

#include "arcade.h"

#define ROUNDS          3
#define NEED            (ROUNDS / 2 + 1)
#define MAX_ROUNDS      9
#define DELAY_MIN_TICKS 20  // 2 s
#define DELAY_MAX_TICKS 50  // 5 s
#define REACTION_TICKS  30  // no press within 3 s is a tie
#define RESULT_TICKS    25  // how long a round's result stays up

#define LEFT  0
#define RIGHT 1
#define NOBODY (-1)

typedef enum { ARMED, FLASH, ROUND_RESULT, OVER } duel_mode_t;

typedef enum {
    WHY_FIRST,       // the winner pressed first after the flash
    WHY_EARLY,       // the loser pressed before the flash
    WHY_BOTH_EARLY,  // both pressed before the flash
    WHY_SAME,        // both pressed at the same instant
    WHY_NOBODY,      // nobody pressed in time
} why_t;

static duel_mode_t mode;
static uint8_t     round_no;
static uint8_t     wins[2];
static uint16_t    wait;       // ticks until the flash
static uint16_t    since;      // ticks since the flash
static uint8_t     pause;      // ticks the round's result has left on screen
static int8_t      winner;     // of the round: LEFT, RIGHT or NOBODY
static why_t       why;
static uint16_t    reaction;   // ticks

static void append(char *out, size_t size, const char *text)
{
    size_t n = strlen(out);
    while (*text && n + 1 < size) {
        out[n++] = *text++;
    }
    out[n] = '\0';
}

static void append_number(char *out, size_t size, uint32_t value)
{
    char digits[16];
    ui_number(digits, value, NULL);
    append(out, size, digits);
}

static void next_round(void)
{
    round_no++;
    wait  = (uint16_t) random_range(DELAY_MIN_TICKS, DELAY_MAX_TICKS + 1);
    since = 0;
    mode  = ARMED;
    ui_dirty();
}

static void start_match(void)
{
    round_no    = 0;
    wins[LEFT]  = 0;
    wins[RIGHT] = 0;
    next_round();
}

static void end_round(int8_t who, why_t reason)
{
    winner = who;
    why    = reason;
    if (who != NOBODY) {
        wins[who]++;
    }
    reaction = since;
    pause    = RESULT_TICKS;
    mode     = ROUND_RESULT;
    ui_dirty();
}

static void after_round(void)
{
    if (wins[LEFT] >= NEED || wins[RIGHT] >= NEED || round_no >= MAX_ROUNDS) {
        mode = OVER;
        input_swallow();
        ui_dirty();
    }
    else {
        next_round();
    }
}

void duel_enter(void)
{
    start_match();
}

void duel_event(event_t event, uint8_t buttons)
{
    if (event == EV_BACK) {
        shell_back();
        return;
    }

    switch (mode) {
        case OVER:
            if (event == EV_SELECT) {
                start_match();
            }
            break;

        case ARMED:
            if (event != EV_DOWN) {
                break;
            }
            if (buttons == BTN_BOTH) {
                end_round(NOBODY, WHY_BOTH_EARLY);
            }
            else {
                end_round((buttons & BTN_LEFT) ? RIGHT : LEFT, WHY_EARLY);
            }
            break;

        case FLASH:
            if (event != EV_DOWN) {
                break;
            }
            if (buttons == BTN_BOTH) {
                end_round(NOBODY, WHY_SAME);
            }
            else {
                end_round((buttons & BTN_LEFT) ? LEFT : RIGHT, WHY_FIRST);
            }
            break;

        case ROUND_RESULT:
            break;
    }
}

void duel_tick(void)
{
    switch (mode) {
        case ARMED:
            if (++since >= wait) {
                since = 0;
                mode  = FLASH;
                ui_dirty();
            }
            break;
        case FLASH:
            if (++since >= REACTION_TICKS) {
                end_round(NOBODY, WHY_NOBODY);
            }
            break;
        case ROUND_RESULT:
            if (pause && --pause == 0) {
                after_round();
            }
            break;
        default:
            break;
    }
}

// ---------------------------------------------------------------------------------------------
// Screens
// ---------------------------------------------------------------------------------------------

// Each player's side of the bar: their arrow and a heart per round won.
static void match_bar(void)
{
    char left[8]  = G_LEFT " ";
    char right[8] = "";
    for (uint8_t i = 0; i < wins[LEFT]; i++) {
        append(left, sizeof(left), G_HEART);
    }
    for (uint8_t i = 0; i < wins[RIGHT]; i++) {
        append(right, sizeof(right), G_HEART);
    }
    append(right, sizeof(right), " " G_RIGHT);

    ui_page();
    ui_bar(left, right);
}

static const char *winner_text(int8_t who)
{
    return who == LEFT ? G_LEFT " wins" : "wins " G_RIGHT;
}

static void draw_result(void)
{
    char line[24] = "";

    match_bar();
    gfx_text_center(GFX_W / 2, 18, winner == NOBODY ? "tie!" : winner_text(winner), 2, true);

    switch (why) {
        case WHY_FIRST:
            append_number(line, sizeof(line), reaction / 10);
            append(line, sizeof(line), ".");
            append_number(line, sizeof(line), reaction % 10);
            append(line, sizeof(line), " s");
            break;
        case WHY_EARLY:
        case WHY_BOTH_EARLY:
            append(line, sizeof(line), "too early!");
            break;
        case WHY_SAME:
            append(line, sizeof(line), "same time!");
            break;
        case WHY_NOBODY:
            append(line, sizeof(line), "zzz...");
            break;
    }
    gfx_text_center(GFX_W / 2, 42, line, 1, true);
}

static void draw_over(void)
{
    char line[24] = "";

    ui_page();
    if (wins[LEFT] >= NEED || wins[RIGHT] >= NEED) {
        gfx_text_center(GFX_W / 2, 8, wins[LEFT] >= NEED ? G_LEFT " wins!" : "wins! " G_RIGHT, 2, true);
    }
    else {
        gfx_text_center(GFX_W / 2, 8, "draw", 2, true);
    }
    append_number(line, sizeof(line), wins[LEFT]);
    append(line, sizeof(line), " " G_HEART " ");
    append_number(line, sizeof(line), wins[RIGHT]);
    gfx_text_center(GFX_W / 2, 34, line, 2, true);
}

void duel_draw(void)
{
    switch (mode) {
        case ARMED:
            match_bar();
            gfx_text_center(GFX_W / 2, 26, "wait...", 2, true);
            break;

        case FLASH:
            // The flash: the whole screen inked, the word cut out of it.
            gfx_clear(true);
            gfx_text_center(GFX_W / 2, 22, "now!", 3, false);
            break;

        case ROUND_RESULT:
            draw_result();
            break;

        case OVER:
            draw_over();
            break;
    }
}
