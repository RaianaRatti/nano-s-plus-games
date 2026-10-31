// Badge Arcade: the games of BadgeOS on a Ledger Nano S Plus, built on the core (core.h).
//
// The pieces:
//   shell.c    the launcher and the switch between games
//   icons.c    the launcher's pictures
//   storage.c  the best scores, in flash
//   run.c      the life of a score game: title, play, pause, crash, game over, best score
//   dodge.c, dice.c, duel.c   the games of BadgeOS
//   snake.c, pong.c, bricks.c, racer.c, cave.c, flip.c, runner.c   the arcade's own games

#pragma once

#include "core.h"

// The launcher's picture for a card (0 .. NB_GAMES): icons.c
void ui_icon(uint8_t card, int x, int y, int scale);

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

void shell_open(app_id_t app);

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
// A run of a score game: run.c
// ---------------------------------------------------------------------------------------------

typedef enum { RUN_READY, RUN_PLAY, RUN_PAUSED, RUN_CRASH, RUN_OVER } run_mode_t;

typedef struct {
    run_mode_t mode;
    best_t     slot;
    uint8_t    ticks;  // left in the countdown or the crash
    bool       new_best;
    uint32_t   score;
    uint32_t   best;
} run_t;

typedef enum {
    RUN_NOTHING,  // the run used the event
    RUN_START,    // a new run begins (after the first, which run_enter starts): set up the world
    RUN_INPUT,    // in play: the event is for the game
} run_action_t;

void         run_enter(run_t *run, best_t slot);
run_action_t run_event(run_t *run, event_t event);
bool         run_tick(run_t *run);  // true when the world should move
void         run_suspend(run_t *run);
void         run_crash(run_t *run);  // still field for a moment, then game over
void         run_over(run_t *run);   // game over at once
void         run_draw_overlay(const run_t *run);  // over the field: countdown or pause sign
void         run_draw_over(const run_t *run);

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
