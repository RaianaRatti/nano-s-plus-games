// Racer: your car drives to the right on a three-lane road. LEFT steers to the lane on its left
// (up the screen), RIGHT to the lane on its right (down). The traffic comes at you, the road gets
// faster, and the score is the distance driven.
//
// The traffic comes in waves: one or two cars side by side, never three, and far enough apart to
// change lanes between them. Pressing both pauses: the lane change the first button made is taken
// back.

#include "arcade.h"

#define FP 16

#define LANES     3
#define LANE_H    18
#define ROAD_TOP  10
#define CAR_W     14
#define CAR_H     8
#define CAR_X     6
#define MAX_CARS  12

// The road's speed in 1/16 pixel per tick, and the gain every 2 s.
#define SPEED_START 40
#define SPEED_GAIN  2
#define SPEED_MAX   144

// The distance from one wave to the next, in pixels, shrinks as the road speeds up but always
// leaves time to change lanes.
#define WAVE_GAP_START 96
#define WAVE_GAP_MIN   44

#define PX_PER_METRE 8

typedef struct {
    int32_t x;  // 1/16 pixels
    uint8_t lane;
} car_t;

// Seen from above, facing right: wheels at the corners, the windscreen ahead of the cabin.
static const uint16_t CAR[CAR_H] = {
    0b00110000001100,
    0b11111111111110,
    0b11110011011111,
    0b11110011011111,
    0b11110011011111,
    0b11110011011111,
    0b11111111111110,
    0b00110000001100,
};

static run_t    run;
static uint8_t  lane;
static uint8_t  lane_before;  // where the car was when the gesture began
static car_t    cars[MAX_CARS];
static uint8_t  nb_cars;
static int32_t  speed;
static int32_t  next_wave;    // pixels until the next wave
static uint32_t distance;     // 1/16 pixels
static uint32_t ticks;
static uint8_t  dash;         // the lane markings' scroll, pixels

static int lane_y(int l)
{
    return ROAD_TOP + l * LANE_H + (LANE_H - CAR_H) / 2;
}

static void start_play(void)
{
    lane      = 1;
    nb_cars   = 0;
    speed     = SPEED_START;
    next_wave = 40;
    distance  = 0;
    ticks     = 0;
    dash      = 0;
}

static void add_car(uint8_t l)
{
    if (nb_cars < MAX_CARS) {
        cars[nb_cars].x    = GFX_W * FP;
        cars[nb_cars].lane = l;
        nb_cars++;
    }
}

static void spawn_wave(void)
{
    uint8_t first = (uint8_t) random_range(0, LANES);
    add_car(first);
    if (random_range(0, 100) < 45) {
        add_car((uint8_t) ((first + 1 + random_range(0, LANES - 1)) % LANES));
    }
    int32_t gap = WAVE_GAP_START - (speed - SPEED_START) / 2;
    if (gap < WAVE_GAP_MIN) {
        gap = WAVE_GAP_MIN;
    }
    next_wave = gap + (int32_t) random_range(0, 24);
}

static void play_tick(void)
{
    ticks++;
    if (ticks % 20 == 0 && speed < SPEED_MAX) {
        speed += SPEED_GAIN;
    }
    distance += (uint32_t) speed;
    dash = (uint8_t) ((dash + speed / FP) % 8);

    next_wave -= speed / FP;
    if (next_wave <= 0) {
        spawn_wave();
    }

    for (int i = nb_cars - 1; i >= 0; i--) {
        cars[i].x -= speed;
        int x = (int) (cars[i].x / FP);
        if (x + CAR_W < 0) {
            cars[i] = cars[--nb_cars];
            continue;
        }
        // One pixel of grace on each side.
        if (cars[i].lane == lane && x < CAR_X + CAR_W - 1 && x + CAR_W - 1 > CAR_X) {
            run_crash(&run);
            return;
        }
    }

    run.score = distance / FP / PX_PER_METRE;
    ui_dirty();
}

static void steer(int way)
{
    int to = lane + way;
    if (to < 0 || to >= LANES) {
        return;
    }
    lane = (uint8_t) to;
    // A car beside you is a crash at once, not at the next tick.
    for (int i = 0; i < nb_cars; i++) {
        int x = (int) (cars[i].x / FP);
        if (cars[i].lane == lane && x < CAR_X + CAR_W - 1 && x + CAR_W - 1 > CAR_X) {
            run_crash(&run);
            return;
        }
    }
}

// ---------------------------------------------------------------------------------------------
// The app
// ---------------------------------------------------------------------------------------------

void racer_enter(void)
{
    run_enter(&run, BEST_RACER);
}

void racer_suspend(void)
{
    run_suspend(&run);
}

void racer_event(event_t event, uint8_t buttons)
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
                lane = lane_before;  // the second button of a pause
            }
            else {
                lane_before = lane;
                steer((buttons & BTN_LEFT) ? -1 : 1);
            }
            ui_dirty();
            break;
        default:
            break;
    }
}

void racer_tick(void)
{
    if (run_tick(&run)) {
        play_tick();
    }
}

static void draw_field(void)
{
    char line[16];
    ui_number(line, run.score, " M");

    ui_page();
    ui_header("RACER", line);

    // The lane markings scroll toward the player; the road's edges are solid.
    for (int l = 1; l < LANES; l++) {
        int y = ROAD_TOP + l * LANE_H - 1;
        for (int x = -dash; x < GFX_W; x += 8) {
            gfx_fill_rect(x, y, 4, 1, true);
        }
    }
    gfx_fill_rect(0, GFX_H - 1, GFX_W, 1, true);

    for (int i = 0; i < nb_cars; i++) {
        int x = (int) (cars[i].x / FP);
        int y = lane_y(cars[i].lane);
        gfx_rect(x, y, CAR_W, CAR_H, true);  // the traffic: outlines
        gfx_fill_rect(x + 4, y + 2, 2, CAR_H - 4, true);
        gfx_fill_rect(x + 9, y + 2, 2, CAR_H - 4, true);
    }
    gfx_sprite(CAR_X, lane_y(lane), CAR, CAR_W, CAR_H, false, true);
}

void racer_draw(void)
{
    switch (run.mode) {
        case RUN_TITLE:
            run_draw_title(&run, "RACER", G_LEFT " UP A LANE  " G_RIGHT " DOWN", " M");
            break;
        case RUN_OVER:
            run_draw_over(&run, "RACER", "DISTANCE", " M");
            break;
        default:
            draw_field();
            if (run.mode == RUN_PAUSED) {
                run_draw_paused();
            }
            break;
    }
}
