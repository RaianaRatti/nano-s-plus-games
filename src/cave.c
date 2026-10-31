// Cave: hold a button to climb, let go to sink. The cave scrolls by, winds up and down and
// narrows, and now and then a rock stands in the middle of it. The score is the distance flown.
//
// Hold one button, not both: both held is HOLD, which ends the run. The cave is a ring of columns SEG pixels wide, each with its ceiling, its floor and
// maybe a rock.

#include "arcade.h"

#define FP 16

#define TOP     10
#define SEG     4
#define NB_SEGS (GFX_W / SEG + 2)

#define COPTER_X 20
#define COPTER_W 11
#define COPTER_H 6

// Vertical speeds in 1/16 pixel per tick (and per tick, per tick).
#define LIFT     9
#define GRAVITY  7
#define RISE_MAX 40
#define FALL_MAX 48

// Scroll speed in 1/16 pixel per tick, gained every 3 s.
#define SPEED_START 40
#define SPEED_GAIN  4
#define SPEED_MAX   88

// The opening narrows by a pixel every NARROW_EVERY columns.
#define GAP_START    42
#define GAP_MIN      22
#define NARROW_EVERY 24
#define ROCK_H       10
#define ROCK_SEGS    2

#define PX_PER_METRE 8

typedef struct {
    uint8_t top;     // the first open row
    uint8_t bottom;  // the first closed row under the opening
    uint8_t rock;    // the rock's top row, 0 for none
} seg_t;

static const uint16_t COPTER[2][COPTER_H] = {
    {
        0b11111111100,
        0b00001000000,
        0b01111100001,
        0b11011111111,
        0b01111100000,
        0b00100100000,
    },
    {
        0b00111110000,
        0b00001000000,
        0b01111100011,
        0b11011111110,
        0b01111100000,
        0b00100100000,
    },
};

static run_t    run;
static seg_t    segs[NB_SEGS];
static uint8_t  first;    // the ring's leftmost column
static int32_t  shift;    // how far the leftmost column has scrolled off, 1/16 pixels
static int32_t  y, vy;    // the copter's top, 1/16 pixels
static int32_t  speed;
static int      centre;   // where the cave is heading
static uint8_t  gap;
static uint16_t made;     // columns made so far
static uint8_t  rock_left;
static uint8_t  rock_y;
static uint32_t distance;
static uint32_t ticks;

static seg_t *seg(int i)
{
    return &segs[(first + i) % NB_SEGS];
}

static void make_seg(seg_t *s)
{
    made++;
    if (made % NARROW_EVERY == 0 && gap > GAP_MIN) {
        gap--;
    }
    if (made > 12) {  // a straight start, then it winds
        centre += (int) random_range(0, 7) - 3;
    }
    int half = gap / 2;
    if (centre - half < TOP + 1) {
        centre = TOP + 1 + half;
    }
    if (centre + half > GFX_H - 1) {
        centre = GFX_H - 1 - half;
    }
    s->top    = (uint8_t) (centre - half);
    s->bottom = (uint8_t) (s->top + gap);
    s->rock   = 0;

    if (rock_left == 0 && made > 30 && random_range(0, 100) < 6) {
        rock_left = ROCK_SEGS;
        rock_y    = (uint8_t) random_range(s->top + 3, s->bottom - ROCK_H - 2);
    }
    if (rock_left) {
        rock_left--;
        s->rock = rock_y;
    }
}

static void start_play(void)
{
    gap       = GAP_START;
    centre    = (TOP + GFX_H) / 2;
    made      = 0;
    rock_left = 0;
    first     = 0;
    shift     = 0;
    for (int i = 0; i < NB_SEGS; i++) {
        make_seg(&segs[i]);
    }
    y        = (centre - COPTER_H / 2) * FP;
    vy       = 0;
    speed    = SPEED_START;
    distance = 0;
    ticks    = 0;
}

static bool crashed(void)
{
    // The copter's box, a pixel inside its picture.
    int top = (int) (y / FP) + 1, bottom = (int) (y / FP) + COPTER_H - 1;
    int left = COPTER_X + 1, right = COPTER_X + COPTER_W - 1;
    if (top < TOP || bottom > GFX_H) {
        return true;
    }
    for (int i = 0; i < NB_SEGS; i++) {
        int sx = i * SEG - (int) (shift / FP);
        if (sx + SEG <= left || sx >= right) {
            continue;
        }
        seg_t *s = seg(i);
        if (top < s->top || bottom > s->bottom) {
            return true;
        }
        if (s->rock && bottom > s->rock && top < s->rock + ROCK_H) {
            return true;
        }
    }
    return false;
}

static void play_tick(void)
{
    ticks++;
    if (ticks % 30 == 0 && speed < SPEED_MAX) {
        speed += SPEED_GAIN;
    }

    uint8_t held = input_held();
    if (held && held != BTN_BOTH) {
        vy = vy - LIFT < -RISE_MAX ? -RISE_MAX : vy - LIFT;
    }
    else {
        vy = vy + GRAVITY > FALL_MAX ? FALL_MAX : vy + GRAVITY;
    }
    y += vy;

    shift += speed;
    while (shift >= SEG * FP) {
        shift -= SEG * FP;
        make_seg(&segs[first]);  // the column that left on the left comes back on the right
        first = (uint8_t) ((first + 1) % NB_SEGS);
    }
    distance += (uint32_t) speed;
    run.score = distance / FP / PX_PER_METRE;

    if (crashed()) {
        run_crash(&run);
    }
    ui_dirty();
}

// ---------------------------------------------------------------------------------------------
// The app
// ---------------------------------------------------------------------------------------------

void cave_enter(void)
{
    run_enter(&run, BEST_CAVE);
    start_play();
}

void cave_suspend(void)
{
    run_suspend(&run);
}

void cave_event(event_t event, uint8_t buttons)
{
    (void) buttons;
    if (run_event(&run, event) == RUN_START) {
        start_play();
    }
}

void cave_tick(void)
{
    if (run_tick(&run)) {
        play_tick();
    }
}

static void draw_field(void)
{
    char line[16];
    ui_number(line, run.score, NULL);

    ui_page();
    ui_bar(NULL, line);
    for (int i = 0; i < NB_SEGS; i++) {
        int    sx = i * SEG - (int) (shift / FP);
        seg_t *s  = seg(i);
        gfx_fill_rect(sx, TOP, SEG, s->top - TOP, true);
        gfx_fill_rect(sx, s->bottom, SEG, GFX_H - s->bottom, true);
        if (s->rock) {
            gfx_fill_rect(sx, s->rock, SEG, ROCK_H, true);
        }
    }
    gfx_sprite(COPTER_X, (int) (y / FP), COPTER[ticks % 2], COPTER_W, COPTER_H, false, true);
}

void cave_draw(void)
{
    if (run.mode == RUN_OVER) {
        run_draw_over(&run);
        return;
    }
    draw_field();
    run_draw_overlay(&run);
}
