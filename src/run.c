// A run: the life of a score game, shared by Dodge, Snake, Bricks, Racer, Cave, Flip and Runner.
//
//   Ready    the field and a 3, 2, 1 countdown. HOLD goes back to the launcher.
//   Play     the game's own controls. BOTH pauses, HOLD goes back (the score still counts).
//   Paused   BOTH resumes, HOLD goes back. The game also pauses when the device locks.
//   Crash    the field stays still for a moment so the player sees what happened.
//   Over     the score and the best. BOTH plays again, HOLD goes back.
//
// The game keeps its world; the run keeps the mode, the score and the best score, and draws the
// screens that are the same in every game. Opening a game starts a run at once.

#include "arcade.h"

#define READY_TICKS 15
#define CRASH_TICKS 8

static void get_ready(run_t *run)
{
    run->mode  = RUN_READY;
    run->ticks = READY_TICKS;
    run->score = 0;
    ui_dirty();
}

void run_enter(run_t *run, best_t slot)
{
    run->slot = slot;
    run->best = storage_best(slot);
    get_ready(run);
}

static void end_run(run_t *run)
{
    run->new_best = run->score > run->best;
    if (run->new_best) {
        run->best = run->score;
        storage_set_best(run->slot, run->best);
    }
}

static void leave(run_t *run)
{
    end_run(run);
    shell_back();
}

void run_over(run_t *run)
{
    end_run(run);
    run->mode = RUN_OVER;
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
        case RUN_READY:
        case RUN_CRASH:
            if (event == EV_BACK) {
                leave(run);
            }
            break;

        case RUN_PLAY:
            if (event == EV_SELECT) {
                run->mode = RUN_PAUSED;
                ui_dirty();
            }
            else if (event == EV_BACK) {
                leave(run);
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
                leave(run);
            }
            break;

        case RUN_OVER:
            if (event == EV_SELECT) {
                get_ready(run);
                return RUN_START;
            }
            if (event == EV_BACK) {
                shell_back();
            }
            break;
    }
    return RUN_NOTHING;
}

bool run_tick(run_t *run)
{
    switch (run->mode) {
        case RUN_READY:
            if (--run->ticks == 0) {
                run->mode = RUN_PLAY;
            }
            ui_dirty();
            break;
        case RUN_PLAY:
            return true;
        case RUN_CRASH:
            if (--run->ticks == 0) {
                run_over(run);
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

// Over the field: the countdown while getting ready, the pause sign while paused.
void run_draw_overlay(const run_t *run)
{
    if (run->mode == RUN_READY) {
        char digit[2] = {(char) ('1' + (run->ticks - 1) * 3 / READY_TICKS), '\0'};
        gfx_bubble(GFX_W / 2 - 14, 20, 28, 30, 2, false);
        gfx_bubble(GFX_W / 2 - 12, 22, 24, 26, 2, true);
        gfx_text_center(GFX_W / 2, 25, digit, 3, false);
    }
    else if (run->mode == RUN_PAUSED) {
        ui_pause_sign();
    }
}

// oops! / the score / a star and the best, with "new!" when this run set it.
void run_draw_over(const run_t *run)
{
    char line[16];

    ui_page();
    gfx_text_center(GFX_W / 2, 4, "oops!", 2, true);
    ui_number(line, run->score, NULL);
    gfx_text_center(GFX_W / 2, 24, line, 3, true);
    if (run->new_best) {
        ui_bubble_text(GFX_W / 2, 51, G_STAR " new! " G_STAR, 1);
    }
    else {
        ui_best(GFX_W / 2, 54, run->best);
    }
}
