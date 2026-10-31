// Dodge (BadgeOS apps/game): blocks fall, LEFT and RIGHT move the ship along the bottom, the
// score is the seconds survived and the speed rises. The high score is kept in flash.
//
// BadgeOS's game had a shop that sold a shield, a sword and paint for tokens. That was the wallet
// half of the game; the arcade keeps only the game, so a run ends at the first hit.
//
// The sizes and speeds are BadgeOS's, scaled from its 320 x 240 screen to 128 x 64. The world moves
// once a tick (10 times a second); a press moves the ship at once, without waiting for the tick.
//
//   Play     hold LEFT or RIGHT to move. The rest (countdown, pause, game over) is run.c.

#include <string.h>

#include "arcade.h"

// Positions are kept in 1/16 of a pixel.
#define FP 16

// How fast the blocks fall, in pixels per second: the start, the gain per second survived, the
// limit. (BadgeOS: 60, 4, 220 on a field 3.5 times as tall.)
#define SPEED_START 17
#define SPEED_GAIN  1
#define SPEED_MAX   62

// A new block every SPAWN_START_MS, less SPAWN_GAIN_MS for each second survived, never under
// SPAWN_MIN_MS. AIM_PERCENT of the blocks fall straight at the ship, so standing still loses.
#define SPAWN_START_MS 900
#define SPAWN_GAIN_MS  15
#define SPAWN_MIN_MS   250
#define AIM_PERCENT    30

// Sizes in pixels; the ship moves PLAYER_STEP pixels a tick (64 pixels a second).
#define PLAYER_W    12
#define PLAYER_H    3
#define PLAYER_STEP 6
#define BLOCK_W     8
#define BLOCK_H     5

#define FIELD_TOP    9
#define FIELD_BOTTOM GFX_H

#define MAX_BLOCKS 24

typedef struct {
    int16_t x;  // pixels
    int32_t y;  // 1/16 pixels
} block_t;

static run_t    run;
static int      px;  // the ship's left edge, pixels
static block_t  blocks[MAX_BLOCKS];
static uint8_t  nb_blocks;
static uint32_t ticks;     // the run's age
static int32_t  spawn_in;  // ms until the next block
static bool     stepped;   // a press already moved the ship this tick

static void start_play(void)
{
    px        = (GFX_W - PLAYER_W) / 2;
    nb_blocks = 0;
    ticks     = 0;
    spawn_in  = 0;
    stepped   = false;
}
static void move(int direction)
{
    px += direction * PLAYER_STEP;
    if (px < 0) {
        px = 0;
    }
    if (px > GFX_W - PLAYER_W) {
        px = GFX_W - PLAYER_W;
    }
}

static int held_direction(void)
{
    uint8_t held = input_held();
    return ((held & BTN_RIGHT) ? 1 : 0) - ((held & BTN_LEFT) ? 1 : 0);
}

static void spawn(void)
{
    if (nb_blocks == MAX_BLOCKS) {
        return;
    }
    int x;
    if (random_range(0, 100) < AIM_PERCENT) {
        x = px + (PLAYER_W - BLOCK_W) / 2;  // straight at the ship
    }
    else {
        x = (int) random_range(0, GFX_W - BLOCK_W + 1);
    }
    if (x < 0) {
        x = 0;
    }
    if (x > GFX_W - BLOCK_W) {
        x = GFX_W - BLOCK_W;
    }
    blocks[nb_blocks].x = (int16_t) x;
    blocks[nb_blocks].y = FIELD_TOP * FP;
    nb_blocks++;
}

static void play_tick(void)
{
    ticks++;

    if (!stepped) {
        move(held_direction());
    }
    stepped = false;

    spawn_in -= TICK_MS;
    if (spawn_in <= 0) {
        spawn();
        int32_t next = SPAWN_START_MS - (int32_t) (SPAWN_GAIN_MS * ticks / (1000 / TICK_MS));
        spawn_in     = next < SPAWN_MIN_MS ? SPAWN_MIN_MS : next;
    }

    // Pixels per second, then 1/16 pixels per tick.
    int32_t speed = SPEED_START + (int32_t) (SPEED_GAIN * ticks / (1000 / TICK_MS));
    if (speed > SPEED_MAX) {
        speed = SPEED_MAX;
    }
    int32_t fall = speed * FP * TICK_MS / 1000;

    int top = FIELD_BOTTOM - PLAYER_H;
    for (int i = nb_blocks - 1; i >= 0; i--) {
        block_t *b = &blocks[i];
        b->y += fall;
        int y = (int) (b->y / FP);
        bool gone = y >= FIELD_BOTTOM;
        bool hit  = !gone && y + BLOCK_H >= top && b->x < px + PLAYER_W && b->x + BLOCK_W > px;
        if (gone || hit) {
            blocks[i] = blocks[--nb_blocks];
        }
        if (hit) {
            run_over(&run);
            return;
        }
    }

    run.score = ticks / (1000 / TICK_MS);
    ui_dirty();
}

// ---------------------------------------------------------------------------------------------
// The app
// ---------------------------------------------------------------------------------------------

void dodge_enter(void)
{
    run_enter(&run, BEST_DODGE);
    start_play();
}

void dodge_suspend(void)
{
    run_suspend(&run);
}

void dodge_event(event_t event, uint8_t buttons)
{
    switch (run_event(&run, event)) {
        case RUN_START:
            start_play();
            break;
        case RUN_INPUT:
            if (event == EV_DOWN && buttons != BTN_BOTH) {
                move((buttons & BTN_RIGHT) ? 1 : -1);
                stepped = true;
                ui_dirty();
            }
            break;
        default:
            break;
    }
}

void dodge_tick(void)
{
    if (run_tick(&run)) {
        play_tick();
    }
}

static void draw_field(void)
{
    char header[16];
    ui_number(header, run.score, NULL);

    ui_page();
    ui_bar(NULL, header);
    for (int i = 0; i < nb_blocks; i++) {
        int y = (int) (blocks[i].y / FP);
        int h = y + BLOCK_H > FIELD_BOTTOM ? FIELD_BOTTOM - y : BLOCK_H;
        gfx_fill_rect(blocks[i].x, y, BLOCK_W, h, true);
    }
    gfx_fill_rect(px, FIELD_BOTTOM - PLAYER_H, PLAYER_W, PLAYER_H, true);
}

void dodge_draw(void)
{
    if (run.mode == RUN_OVER) {
        run_draw_over(&run);
        return;
    }
    draw_field();
    run_draw_overlay(&run);
}
