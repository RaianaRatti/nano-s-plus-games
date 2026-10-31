// Flip: you run along the floor or the ceiling, and a press of either button turns gravity around
// and sends you across to the other side. Spikes and holes come along on both sides; be on the
// other side when they pass. The score is the distance run.
//
// You can only flip with your feet on the ground, and crossing takes a moment, so the timing is the
// game. In the air you are safe from spikes, but a hole under you when you land is a fall.
//
// The track is a ring of tiles TILE pixels wide. It is made in stretches: a safe run, then a hazard
// on one side. A hazard on the other side from the last one comes only after a run long enough to
// cross at the current speed.

#include "arcade.h"

#define FP 16

#define TILE      4
#define NB_TILES  (GFX_W / TILE + 2)
#define CEIL_Y    10  // the ceiling's band: 10, 11, 12
#define BAND      3
#define FLOOR_Y   (GFX_H - BAND)
#define SPIKE_H   4

#define RUNNER_X 20
#define RUNNER_W 5
#define RUNNER_H 7

// Vertical speeds in 1/16 pixel per tick, and the pull per tick.
#define PUSH     16
#define PULL     40
#define FALL_MAX 128

// Scroll speed in 1/16 pixel per tick, gained every 3 s.
#define SPEED_START 44
#define SPEED_GAIN  4
#define SPEED_MAX   96

// A crossing takes about this many ticks; a hazard on the other side waits for it.
#define CROSS_TICKS 8

#define PX_PER_METRE 8

// What a tile holds.
#define F_HOLE  0x01
#define F_SPIKE 0x02
#define C_HOLE  0x04
#define C_SPIKE 0x08

static const uint16_t RUNNER[2][RUNNER_H] = {
    {0b01110, 0b01110, 0b00100, 0b11111, 0b00100, 0b01010, 0b10001},
    {0b01110, 0b01110, 0b00100, 0b01110, 0b10101, 0b00100, 0b01010},
};

static run_t    run;
static uint8_t  tiles[NB_TILES];
static uint8_t  first;
static int32_t  shift;
static int32_t  y, vy;      // the runner's top, 1/16 pixels
static int8_t   pull;       // +1 toward the floor, -1 toward the ceiling
static bool     grounded;
static bool     flipped;    // this gesture flipped: the second button of a pause takes it back
static int32_t  speed;
static uint8_t  run_left;   // safe tiles before the next hazard
static uint8_t  hazard_left;
static uint8_t  hazard;     // the flags of the hazard being made
static uint8_t  last_side;  // F_HOLE | F_SPIKE or C_HOLE | C_SPIKE: the side of the last hazard
static uint16_t made;
static uint32_t distance;
static uint32_t ticks;

static uint8_t *tile(int i)
{
    return &tiles[(first + i) % NB_TILES];
}

static void plan_hazard(void)
{
    bool floor_side = random_range(0, 2) == 0;
    bool hole       = random_range(0, 3) == 0;
    hazard          = floor_side ? (hole ? F_HOLE : F_SPIKE) : (hole ? C_HOLE : C_SPIKE);
    hazard_left     = (uint8_t) random_range(2, hole ? 5 : 7);

    uint8_t side = floor_side ? (F_HOLE | F_SPIKE) : (C_HOLE | C_SPIKE);
    int     safe = 3;
    if (side != last_side) {
        // Room to cross: the flight, the runner's width and a margin.
        safe = (int) ((speed * CROSS_TICKS / FP + RUNNER_W + 6) / TILE);
    }
    run_left  = (uint8_t) (safe + (int) random_range(0, 6));
    last_side = side;
}

static uint8_t make_tile(void)
{
    made++;
    if (made < 24) {
        return 0;  // a clear start
    }
    if (run_left) {
        run_left--;
        return 0;
    }
    if (hazard_left) {
        hazard_left--;
        return hazard;
    }
    plan_hazard();
    return 0;
}

static void start_play(void)
{
    made        = 0;
    first       = 0;
    shift       = 0;
    speed       = SPEED_START;
    last_side   = F_HOLE | F_SPIKE;
    run_left    = 0;
    hazard_left = 0;
    plan_hazard();
    for (int i = 0; i < NB_TILES; i++) {
        tiles[i] = make_tile();
    }
    pull     = 1;
    y        = (FLOOR_Y - RUNNER_H) * FP;
    vy       = 0;
    grounded = true;
    flipped  = false;
    distance = 0;
    ticks    = 0;
}

// The tiles under the runner's feet (the given flags) are all solid.
static bool supported(uint8_t holes)
{
    bool any = false;
    for (int i = 0; i < NB_TILES; i++) {
        int sx = i * TILE - (int) (shift / FP);
        if (sx + TILE <= RUNNER_X + 1 || sx >= RUNNER_X + RUNNER_W - 1) {
            continue;
        }
        if (!(*tile(i) & holes)) {
            any = true;  // one solid tile under a foot is enough
        }
    }
    return any;
}

static bool on_spikes(void)
{
    int top = (int) (y / FP), bottom = top + RUNNER_H;
    for (int i = 0; i < NB_TILES; i++) {
        int sx = i * TILE - (int) (shift / FP);
        if (sx + TILE - 1 <= RUNNER_X || sx + 1 >= RUNNER_X + RUNNER_W) {
            continue;
        }
        uint8_t t = *tile(i);
        if ((t & F_SPIKE) && bottom > FLOOR_Y - SPIKE_H + 1) {
            return true;
        }
        if ((t & C_SPIKE) && top < CEIL_Y + BAND + SPIKE_H - 1) {
            return true;
        }
    }
    return false;
}

static void flip(void)
{
    if (!grounded) {
        return;
    }
    pull     = (int8_t) -pull;
    vy       = pull * PUSH;
    grounded = false;
    flipped  = true;
}

static void play_tick(void)
{
    ticks++;
    if (ticks % 30 == 0 && speed < SPEED_MAX) {
        speed += SPEED_GAIN;
    }

    shift += speed;
    while (shift >= TILE * FP) {
        shift -= TILE * FP;
        tiles[first] = make_tile();
        first        = (uint8_t) ((first + 1) % NB_TILES);
    }
    distance += (uint32_t) speed;
    run.score = distance / FP / PX_PER_METRE;

    // Falling toward the pull; landing on the band, unless there is a hole.
    vy += pull * PULL;
    if (vy > FALL_MAX) {
        vy = FALL_MAX;
    }
    if (vy < -FALL_MAX) {
        vy = -FALL_MAX;
    }
    int32_t was   = y;
    int32_t floor = (FLOOR_Y - RUNNER_H) * FP;
    int32_t ceil  = (CEIL_Y + BAND) * FP;
    y += vy;
    grounded = false;
    // A landing is a crossing of the band's surface this tick, with ground under the feet.
    if (pull > 0 && was <= floor && y >= floor && supported(F_HOLE)) {
        y        = floor;
        vy       = 0;
        grounded = true;
    }
    if (pull < 0 && was >= ceil && y <= ceil && supported(C_HOLE)) {
        y        = ceil;
        vy       = 0;
        grounded = true;
    }

    // Through a hole and off the screen, or onto spikes.
    if (y > GFX_H * FP || y < (CEIL_Y - RUNNER_H) * FP || on_spikes()) {
        run_crash(&run);
    }
    ui_dirty();
}

// ---------------------------------------------------------------------------------------------
// The app
// ---------------------------------------------------------------------------------------------

void flip_enter(void)
{
    run_enter(&run, BEST_FLIP);
    start_play();
}

void flip_suspend(void)
{
    run_suspend(&run);
}

void flip_event(event_t event, uint8_t buttons)
{
    switch (run_event(&run, event)) {
        case RUN_START:
            start_play();
            break;
        case RUN_INPUT:
            if (event != EV_DOWN || buttons == BTN_BOTH) {
                break;
            }
            if (input_held() == BTN_BOTH) {
                // The second button of a pause: take back the flip if it is still in the air.
                if (flipped && !grounded) {
                    pull = (int8_t) -pull;
                    vy   = 0;
                }
            }
            else {
                flipped = false;
                flip();
            }
            ui_dirty();
            break;
        default:
            break;
    }
}

void flip_tick(void)
{
    if (run_tick(&run)) {
        play_tick();
    }
}

static void draw_band(int sx, int band_y, bool hole, bool spike, bool up)
{
    if (hole) {
        return;
    }
    gfx_fill_rect(sx, band_y, TILE, BAND, true);
    if (spike) {
        // A spike: 1, 3 and 1 pixels wide as it rises.
        int base = up ? band_y - 1 : band_y + BAND;
        int step = up ? -1 : 1;
        gfx_fill_rect(sx, base, TILE - 1, 1, true);
        gfx_fill_rect(sx, base + step, TILE - 1, 1, true);
        gfx_fill_rect(sx + 1, base + 2 * step, 1, 1, true);
        gfx_fill_rect(sx + 1, base + 3 * step, 1, 1, true);
    }
}

static void draw_field(void)
{
    char line[16];
    ui_number(line, run.score, NULL);

    ui_page();
    ui_bar(NULL, line);
    for (int i = 0; i < NB_TILES; i++) {
        int     sx = i * TILE - (int) (shift / FP);
        uint8_t t  = *tile(i);
        draw_band(sx, CEIL_Y, t & C_HOLE, t & C_SPIKE, false);
        draw_band(sx, FLOOR_Y, t & F_HOLE, t & F_SPIKE, true);
    }
    // On the ceiling the runner hangs upside down; the legs move while it runs.
    uint8_t frame = grounded ? (uint8_t) (ticks / 2 % 2) : 1;
    gfx_sprite(RUNNER_X, (int) (y / FP), RUNNER[frame], RUNNER_W, RUNNER_H, pull < 0, true);
}

void flip_draw(void)
{
    if (run.mode == RUN_OVER) {
        run_draw_over(&run);
        return;
    }
    draw_field();
    run_draw_overlay(&run);
}
