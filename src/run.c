// A run: the life of a score game, shared by Dodge, Snake, Bricks, Racer, Cave, Flip and Runner.
//
//   Title    BOTH plays, HOLD goes back to the launcher.
//   Play     the game's own controls. BOTH pauses, HOLD ends the run (the score still counts).
//   Paused   BOTH resumes, HOLD ends the run. The game also pauses when the device locks.
//   Crash    the field stays still for a moment so the player sees what happened.
//   Over     any button, or 3 s, returns to the title.
//
// The game keeps its world; the run keeps the mode, the score and the best score, and draws the
// screens that are the same in every game.

#include "arcade.h"

#define CRASH_TICKS 8
#define OVER_TICKS  30

void run_enter(run_t *run, best_t slot)
{
    run->slot = slot;
    run->best = storage_best(slot);
    run->mode = RUN_TITLE;
    ui_dirty();
}

static void to_title(run_t *run)
{
    run->mode = RUN_TITLE;
    ui_dirty();
}

static void end_run(run_t *run)
{
    run->new_best = run->score > run->best;
    if (run->new_best) {
        run->best = run->score;
        storage_set_best(run->slot, run->best);
    }
}

void run_over(run_t *run)
{
    end_run(run);
    run->mode  = RUN_OVER;
    run->ticks = OVER_TICKS;
    input_swallow();  // the hand is still on a button: its release must not leave this screen
    ui_dirty();
}

void run_crash(run_t *run)
{
    run->mode  = RUN_CRASH;
    run->ticks = CRASH_TICKS;
    input_swallow();
    ui_dirty();
}

run_action_t run_event(run_t *run, event_t event)
{
    switch (run->mode) {
        case RUN_TITLE:
            if (event == EV_SELECT) {
                run->mode  = RUN_PLAY;
                run->score = 0;
                ui_dirty();
                return RUN_START;
            }
            if (event == EV_BACK) {
                shell_back();
            }
            break;

        case RUN_PLAY:
            if (event == EV_SELECT) {
                run->mode = RUN_PAUSED;
                ui_dirty();
            }
            else if (event == EV_BACK) {
                end_run(run);
                to_title(run);
            }
            else {
                return RUN_INPUT;
            }
            break;

        case RUN_PAUSED:
            if (event == EV_SELECT) {
                run->mode = RUN_PLAY;
                ui_dirty();
            }
            else if (event == EV_BACK) {
                end_run(run);
                to_title(run);
            }
            break;

        case RUN_CRASH:
            break;

        case RUN_OVER:
            if (event != EV_DOWN) {
                to_title(run);
            }
            break;
    }
    return RUN_NOTHING;
}

bool run_tick(run_t *run)
{
    switch (run->mode) {
        case RUN_PLAY:
            return true;
        case RUN_CRASH:
            if (--run->ticks == 0) {
                run_over(run);
            }
            break;
        case RUN_OVER:
            if (run->ticks && --run->ticks == 0) {
                to_title(run);
            }
            break;
        default:
            break;
    }
    return false;
}

void run_suspend(run_t *run)
{
    if (run->mode == RUN_PLAY) {
        run->mode = RUN_PAUSED;
        ui_dirty();
    }
}

// ---------------------------------------------------------------------------------------------
// The screens every game shares
// ---------------------------------------------------------------------------------------------

// The best score with its unit, and " · NEW" when this run set it.
static void best_text(char *out, const run_t *run, const char *unit, bool mark_new)
{
    char suffix[12] = "";
    int  n          = 0;
    for (; unit && *unit && n < 4; unit++) {
        suffix[n++] = *unit;
    }
    if (mark_new) {
        const char *mark = " " G_DOT " NEW";
        for (; *mark; mark++) {
            suffix[n++] = *mark;
        }
    }
    suffix[n] = '\0';
    ui_number(out, run->best, suffix);
}

void run_draw_title(const run_t *run, const char *name, const char *hint, const char *unit)
{
    char line[16];

    ui_page();
    ui_header(UI_HEADER, name);
    gfx_text_center(GFX_W / 2, 14, name, 2, true);
    best_text(line, run, unit, false);
    gfx_text(28, 33, "BEST", 1, true);
    gfx_text_right(100, 33, line, 1, true);
    gfx_text_center(GFX_W / 2, 43, hint, 1, true);
    ui_footer("BOTH:PLAY", "HOLD:BACK");
}

void run_draw_paused(void)
{
    gfx_fill_rect(14, 20, GFX_W - 28, 28, false);
    gfx_rect(14, 20, GFX_W - 28, 28, true);
    gfx_text_center(GFX_W / 2, 24, "PAUSED", 1, true);
    gfx_text_center(GFX_W / 2, 33, "BOTH:RESUME", 1, true);
    gfx_text_center(GFX_W / 2, 40, "HOLD:END RUN", 1, true);
}

void run_draw_over(const run_t *run, const char *name, const char *label, const char *unit)
{
    char line[16];

    ui_page();
    ui_header(UI_HEADER, name);
    gfx_text_center(GFX_W / 2, 13, "GAME OVER", 2, true);
    ui_number(line, run->score, unit);
    gfx_text(16, 33, label, 1, true);
    gfx_text_right(112, 33, line, 1, true);
    gfx_text(16, 43, "BEST", 1, true);
    best_text(line, run, unit, run->new_best);
    gfx_text_right(112, 43, line, 1, true);
    ui_footer("ANY KEY:TITLE", NULL);
}
