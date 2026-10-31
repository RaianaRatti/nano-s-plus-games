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
//   Title    BOTH starts, HOLD goes back to the launcher.
//   Playing  HOLD (both players, together) leaves the duel.
//   Over     BOTH plays again, HOLD returns to the title.

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

typedef enum { TITLE, ARMED, FLASH, ROUND_RESULT, OVER } duel_mode_t;

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

static void to_title(void)
{
    mode = TITLE;
    ui_dirty();
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
    to_title();
}

void duel_event(event_t event, uint8_t buttons)
{
    if (event == EV_BACK) {
        if (mode == TITLE) {
            shell_back();
        }
        else {
            to_title();
        }
        return;
    }

    switch (mode) {
        case TITLE:
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

static const char *side(int8_t who)
{
    return who == LEFT ? "LEFT" : "RIGHT";
}

// The round in the header, the score in the footer, each player under their own button.
static void match_frame(void)
{
    char round_text[16] = "ROUND ";
    char left[16]       = G_LEFT " LEFT ";
    char right[16]      = "";

    append_number(round_text, sizeof(round_text), round_no);
    append_number(left, sizeof(left), wins[LEFT]);
    append_number(right, sizeof(right), wins[RIGHT]);
    append(right, sizeof(right), " RIGHT " G_RIGHT);

    ui_page();
    ui_header(round_text, "BEST OF 3");
    ui_footer(left, right);
}

static void draw_result(void)
{
    char line[24] = "";

    match_frame();
    gfx_text_center(GFX_W / 2, 13, winner == NOBODY ? "TIE" : side(winner), 2, true);

    switch (why) {
        case WHY_FIRST:
            append(line, sizeof(line), "WINS THE ROUND");
            break;
        case WHY_EARLY:
            append(line, sizeof(line), side(winner == LEFT ? RIGHT : LEFT));
            append(line, sizeof(line), " WAS TOO EARLY");
            break;
        case WHY_BOTH_EARLY:
            append(line, sizeof(line), "BOTH TOO EARLY");
            break;
        case WHY_SAME:
            append(line, sizeof(line), "SAME INSTANT");
            break;
        case WHY_NOBODY:
            append(line, sizeof(line), "NOBODY PRESSED");
            break;
    }
    gfx_text_center(GFX_W / 2, 32, line, 1, true);

    line[0] = '\0';
    if (why == WHY_FIRST) {
        if (reaction == 0) {
            append(line, sizeof(line), "IN UNDER 0.1 S");
        }
        else {
            append(line, sizeof(line), "IN ");
            append_number(line, sizeof(line), reaction / 10);
            append(line, sizeof(line), ".");
            append_number(line, sizeof(line), reaction % 10);
            append(line, sizeof(line), " S");
        }
    }
    else if (winner == NOBODY) {
        append(line, sizeof(line), "PLAYED AGAIN");
    }
    gfx_text_center(GFX_W / 2, 42, line, 1, true);
}

static void draw_over(void)
{
    char line[24] = "";

    ui_page();
    ui_header(UI_HEADER, "DUEL");
    if (wins[LEFT] >= NEED) {
        gfx_text_center(GFX_W / 2, 13, "LEFT WINS", 2, true);
    }
    else if (wins[RIGHT] >= NEED) {
        gfx_text_center(GFX_W / 2, 13, "RIGHT WINS", 2, true);
    }
    else {
        gfx_text_center(GFX_W / 2, 13, "DRAW", 2, true);
    }

    append(line, sizeof(line), "LEFT ");
    append_number(line, sizeof(line), wins[LEFT]);
    append(line, sizeof(line), " " G_DOT " ");
    append_number(line, sizeof(line), wins[RIGHT]);
    append(line, sizeof(line), " RIGHT");
    gfx_text_center(GFX_W / 2, 32, line, 1, true);

    line[0] = '\0';
    append(line, sizeof(line), "AFTER ");
    append_number(line, sizeof(line), round_no);
    append(line, sizeof(line), round_no == 1 ? " ROUND" : " ROUNDS");
    gfx_text_center(GFX_W / 2, 42, line, 1, true);

    ui_footer("BOTH:AGAIN", "HOLD:BACK");
}

void duel_draw(void)
{
    switch (mode) {
        case TITLE:
            ui_page();
            ui_header(UI_HEADER, "DUEL");
            gfx_text_center(GFX_W / 2, 13, "DUEL", 2, true);
            gfx_text_center(GFX_W / 2, 32, G_LEFT " LEFT  VS  RIGHT " G_RIGHT, 1, true);
            gfx_text_center(GFX_W / 2, 42, "PRESS AT THE FLASH", 1, true);
            ui_footer("BOTH:START", "HOLD:BACK");
            break;

        case ARMED:
            match_frame();
            gfx_text_center(GFX_W / 2, 14, "WAIT...", 2, true);
            gfx_text_center(GFX_W / 2, 33, "PRESS AT THE FLASH", 1, true);
            gfx_text_center(GFX_W / 2, 43, "TOO EARLY LOSES", 1, true);
            break;

        case FLASH:
            // The flash: the whole screen lit, the words cut out of it.
            gfx_clear(true);
            gfx_text_center(GFX_W / 2, 18, "PRESS!", 3, false);
            break;

        case ROUND_RESULT:
            draw_result();
            break;

        case OVER:
            draw_over();
            break;
    }
}
