// Runner: a night run over the rooftops. RIGHT jumps (hold it to jump higher), hold LEFT to duck,
// and LEFT in the air drops you faster. Jump the crates, duck under the low bats, stay under the
// high ones, and grab the coins floating over the crates. The score is the distance plus ten per
// coin.
//
// The obstacles come one at a time, spaced so that a jump from the last one lands with room to
// spare at the current speed.

#include "arcade.h"

#define FP 16

#define GROUND   56  // the rooftop's edge: the first row under the runner's feet
#define RUNNER_X 14
#define STAND_W  7
#define STAND_H  12
#define DUCK_W   9
#define DUCK_H   7

// Vertical speeds in 1/16 pixel per tick, and the pulls per tick.
#define JUMP      96
#define PULL_HELD 12  // rising with RIGHT held
#define PULL      20
#define PULL_DROP 40  // LEFT held in the air

// Scroll speed in 1/16 pixel per tick, gained every 3 s.
#define SPEED_START 48
#define SPEED_GAIN  4
#define SPEED_MAX   112

#define AIR_TICKS   14  // a full jump, for the spacing
#define MAX_THINGS  6
#define COIN_POINTS 10
#define PX_PER_POINT 8

typedef enum { CRATE, CRATES, BAT_LOW, BAT_HIGH, COIN } kind_t;

typedef struct {
    int32_t x;  // 1/16 pixels
    uint8_t kind;
    bool    taken;
} thing_t;

static const uint16_t STAND[2][STAND_H] = {
    {0b0011100, 0b0011100, 0b0001000, 0b0111110, 0b1011001, 0b0011000,
     0b0011100, 0b0010100, 0b0100010, 0b0100011, 0b1100000, 0b0000000},
    {0b0011100, 0b0011100, 0b0001000, 0b0011100, 0b0011110, 0b0101000,
     0b0011000, 0b0011100, 0b0001010, 0b0011010, 0b0010000, 0b0011000},
};

static const uint16_t JUMPING[STAND_H] = {
    0b0011100, 0b0011100, 0b1001001, 0b0111110, 0b0011000, 0b0011000,
    0b0011100, 0b0110110, 0b1100011, 0b0000000, 0b0000000, 0b0000000,
};

static const uint16_t DUCKING[DUCK_H] = {
    0b000000111, 0b000000111, 0b011111110, 0b111111000,
    0b001110000, 0b011011000, 0b110001100,
};

static const uint16_t BAT[2][5] = {
    {0b100000001, 0b110000011, 0b011101110, 0b001111100, 0b000101000},
    {0b000000000, 0b000101000, 0b011111110, 0b110111011, 0b100000001},
};

static run_t    run;
static thing_t  things[MAX_THINGS];
static uint8_t  nb_things;
static int32_t  height, vy;  // the feet above the ground, 1/16 pixels; up is positive
static bool     ducking;
static bool     jumped;      // this gesture jumped: the second button of a pause takes it back
static int32_t  speed;
static int32_t  next_in;     // pixels until the next obstacle
static uint32_t distance;
static uint32_t coins;
static uint32_t ticks;
static uint8_t  roof;        // the rooftop texture's scroll

// Each kind's box: x offset and width, and its rows as heights above the ground (low, high).
static void box(uint8_t kind, int *w, int *low, int *high)
{
    switch (kind) {
        case CRATE:
            *w = 8, *low = 0, *high = 8;
            break;
        case CRATES:
            *w = 16, *low = 0, *high = 8;
            break;
        case BAT_LOW:
            *w = 9, *low = 9, *high = 14;
            break;
        case BAT_HIGH:
            *w = 9, *low = 17, *high = 22;
            break;
        default:  // COIN
            *w = 4, *low = 14, *high = 18;
            break;
    }
}

static void add(uint8_t kind, int32_t x)
{
    if (nb_things < MAX_THINGS) {
        things[nb_things].x     = x;
        things[nb_things].kind  = kind;
        things[nb_things].taken = false;
        nb_things++;
    }
}

static void spawn(void)
{
    uint32_t roll = random_range(0, 100);
    int32_t  x    = (GFX_W + 4) * FP;
    if (roll < 40) {
        add(CRATE, x);
        if (random_range(0, 2)) {
            add(COIN, x + 2 * FP);
        }
    }
    else if (roll < 60) {
        add(CRATES, x);
        add(COIN, x + 6 * FP);
    }
    else if (roll < 85 && ticks > 60) {
        add(BAT_LOW, x);
    }
    else {
        add(BAT_HIGH, x);
    }
    // Room to land and take off again, and a little more at random.
    next_in = speed * AIR_TICKS / FP + 16 + (int32_t) random_range(0, 48);
}

static void start_play(void)
{
    nb_things = 0;
    height    = 0;
    vy        = 0;
    ducking   = false;
    speed     = SPEED_START;
    next_in   = 60;
    distance  = 0;
    coins     = 0;
    ticks     = 0;
    roof      = 0;
}

static void play_tick(void)
{
    ticks++;
    if (ticks % 30 == 0 && speed < SPEED_MAX) {
        speed += SPEED_GAIN;
    }

    uint8_t held = input_held();
    bool    air  = height > 0 || vy > 0;
    ducking      = !air && held == BTN_LEFT;
    if (air) {
        int32_t pull = PULL;
        if (held == BTN_LEFT) {
            pull = PULL_DROP;
        }
        else if (vy > 0 && (held & BTN_RIGHT)) {
            pull = PULL_HELD;
        }
        vy -= pull;
        height += vy;
        if (height <= 0) {
            height = 0;
            vy     = 0;
        }
    }

    distance += (uint32_t) speed;
    roof = (uint8_t) ((roof + speed / FP) % 16);
    next_in -= speed / FP;
    if (next_in <= 0) {
        spawn();
    }

    // The runner's box, a pixel inside the picture.
    int r_low  = (int) (height / FP);
    int r_high = r_low + (ducking ? DUCK_H : STAND_H) - 1;
    int r_left = RUNNER_X + 1;
    int r_right = RUNNER_X + (ducking ? DUCK_W : STAND_W) - 1;

    for (int i = nb_things - 1; i >= 0; i--) {
        thing_t *t = &things[i];
        t->x -= speed;
        int w, low, high;
        box(t->kind, &w, &low, &high);
        int x = (int) (t->x / FP);
        if (x + w < 0) {
            things[i] = things[--nb_things];
            continue;
        }
        bool touch = x < r_right && x + w > r_left && low < r_high && high > r_low;
        if (!touch || t->taken) {
            continue;
        }
        if (t->kind == COIN) {
            t->taken = true;
            coins++;
        }
        else {
            run_crash(&run);
            return;
        }
    }

    run.score = distance / FP / PX_PER_POINT + coins * COIN_POINTS;
    ui_dirty();
}

static void jump(void)
{
    if (height == 0 && vy == 0) {
        vy     = JUMP;
        jumped = true;
    }
}

// ---------------------------------------------------------------------------------------------
// The app
// ---------------------------------------------------------------------------------------------

void runner_enter(void)
{
    run_enter(&run, BEST_RUNNER);
    start_play();
}

void runner_suspend(void)
{
    run_suspend(&run);
}

void runner_event(event_t event, uint8_t buttons)
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
                if (jumped) {  // the second button of a pause
                    height = 0;
                    vy     = 0;
                }
            }
            else {
                jumped = false;
                if (buttons == BTN_RIGHT) {  // RIGHT jumps the instant it goes down
                    jump();
                }
            }
            ui_dirty();
            break;
        default:
            break;
    }
}

void runner_tick(void)
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

    // A moon, and the rooftop: its edge, and the joints of its tiles going by.
    gfx_round_rect(108, 14, 7, 7, true);
    gfx_fill_rect(109, 15, 5, 5, true);
    gfx_fill_rect(110, 16, 2, 2, false);
    gfx_fill_rect(0, GROUND, GFX_W, 1, true);
    for (int x = -roof; x < GFX_W; x += 16) {
        gfx_fill_rect(x, GROUND + 3, 6, 1, true);
        gfx_fill_rect(x + 9, GROUND + 6, 4, 1, true);
    }

    for (int i = 0; i < nb_things; i++) {
        thing_t *t = &things[i];
        int      w, low, high;
        box(t->kind, &w, &low, &high);
        int x   = (int) (t->x / FP);
        int top = GROUND - high;
        switch (t->kind) {
            case CRATE:
            case CRATES:
                for (int c = 0; c < w; c += 8) {
                    gfx_rect(x + c, top, 8, 8, true);
                    for (int d = 1; d < 7; d++) {  // a cross brace
                        gfx_pixel(x + c + d, top + d, true);
                    }
                }
                break;
            case BAT_LOW:
            case BAT_HIGH:
                gfx_sprite(x, top, BAT[ticks / 2 % 2], 9, 5, false, true);
                break;
            default:
                if (!t->taken) {
                    gfx_round_rect(x, top, 4, 4, true);
                }
                break;
        }
    }

    int feet = GROUND - (int) (height / FP);
    if (ducking) {
        gfx_sprite(RUNNER_X, feet - DUCK_H, DUCKING, DUCK_W, DUCK_H, false, true);
    }
    else if (height > 0) {
        gfx_sprite(RUNNER_X, feet - STAND_H, JUMPING, STAND_W, STAND_H, false, true);
    }
    else {
        gfx_sprite(RUNNER_X, feet - STAND_H, STAND[ticks / 2 % 2], STAND_W, STAND_H, false, true);
    }
}

void runner_draw(void)
{
    if (run.mode == RUN_OVER) {
        run_draw_over(&run);
        return;
    }
    draw_field();
    run_draw_overlay(&run);
}
