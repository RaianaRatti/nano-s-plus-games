// Bricks: hold LEFT or RIGHT to move the paddle, keep the ball up and break every brick. A
// cleared wall brings the next one, a little faster. Three balls; the higher rows score more.
//
// Where the ball lands on the paddle sets the angle it leaves at (eight zones, from steep in the
// middle to flat at the ends), so the player aims. The ball moves at most one pixel at a time, one
// axis then the other, which tells a hit on a brick's side from a hit on its top or bottom.

#include "arcade.h"

#define FP 16

#define TOP      10
#define PADDLE_W 16
#define PADDLE_H 2
#define PADDLE_Y 60
#define PADDLE_STEP 5
#define BALL     2

#define COLS    10
#define ROWS    5
#define BRICK_W 11
#define BRICK_H 3
#define PITCH_X 12
#define PITCH_Y 4
#define WALL_X  ((GFX_W - (COLS * PITCH_X - 1)) / 2)
#define WALL_Y  14

#define BALLS       3
#define SERVE_TICKS 12

// The ball's speed in 1/16 pixel per tick: about 3 pixels on the first wall.
#define SPEED_START 44
#define SPEED_GAIN  6
#define SPEED_MAX   80

// The walls, one row of COLS bits per line, top row first; the leftmost brick is the high bit.
#define NB_WALLS 4
static const uint16_t WALLS[NB_WALLS][ROWS] = {
    {0x3FF, 0x3FF, 0x3FF, 0x3FF, 0x3FF},  // full
    {0x030, 0x078, 0x0FC, 0x1FE, 0x3FF},  // pyramid
    {0x2AA, 0x155, 0x2AA, 0x155, 0x2AA},  // checkerboard
    {0x3FF, 0x201, 0x2DD, 0x201, 0x3FF},  // frame
};

// sin and cos of the eight leaving angles (from 60 degrees left of straight up to 60 right),
// times 64.
static const int8_t ZONE_X[8] = {-55, -45, -32, -17, 17, 32, 45, 55};
static const int8_t ZONE_Y[8] = {32, 45, 55, 62, 62, 55, 45, 32};

static run_t    run;
static uint16_t bricks[ROWS];
static uint8_t  level;  // from 1
static uint8_t  balls;  // in play and in reserve
static int      px;
static int32_t  ball_x, ball_y, vx, vy;
static int32_t  speed;
static uint8_t  serving;  // ticks left with the ball on the paddle
static bool     stepped;

static int32_t abs32(int32_t v)
{
    return v < 0 ? -v : v;
}

static void launch(int zone)
{
    vx = speed * ZONE_X[zone] / 64;
    vy = -speed * ZONE_Y[zone] / 64;
}

static void serve(void)
{
    serving = SERVE_TICKS;
    ball_x  = (px + PADDLE_W / 2 - BALL / 2) * FP;
    ball_y  = (PADDLE_Y - BALL) * FP;
}

static void load_wall(void)
{
    for (int r = 0; r < ROWS; r++) {
        bricks[r] = WALLS[(level - 1) % NB_WALLS][r];
    }
    speed = SPEED_START + SPEED_GAIN * (level - 1);
    if (speed > SPEED_MAX) {
        speed = SPEED_MAX;
    }
    serve();
}

static void start_play(void)
{
    level   = 1;
    balls   = BALLS;
    px      = (GFX_W - PADDLE_W) / 2;
    stepped = false;
    load_wall();
}

static bool wall_cleared(void)
{
    for (int r = 0; r < ROWS; r++) {
        if (bricks[r]) {
            return false;
        }
    }
    return true;
}

// Breaks every brick under the ball. Returns true if there was one.
static bool break_bricks(void)
{
    int  x0  = (int) (ball_x / FP), y0 = (int) (ball_y / FP);
    bool hit = false;
    for (int r = 0; r < ROWS; r++) {
        int top = WALL_Y + r * PITCH_Y;
        if (y0 + BALL <= top || y0 >= top + BRICK_H) {
            continue;
        }
        for (int c = 0; c < COLS; c++) {
            uint16_t bit  = (uint16_t) (1u << (COLS - 1 - c));
            int      left = WALL_X + c * PITCH_X;
            if ((bricks[r] & bit) && x0 + BALL > left && x0 < left + BRICK_W) {
                bricks[r] &= (uint16_t) ~bit;
                run.score += (uint32_t) (ROWS - r) * 10;
                hit = true;
            }
        }
    }
    return hit;
}

static void bounce_on_paddle(void)
{
    int center = (int) (ball_x / FP) + BALL / 2 - px;  // 0 .. PADDLE_W, wider at the edges
    int zone   = center * 8 / PADDLE_W;
    if (zone < 0) {
        zone = 0;
    }
    if (zone > 7) {
        zone = 7;
    }
    launch(zone);
}

// One pixel (at most) of the ball's path. Returns false when the ball is lost.
static bool ball_step(int32_t dx, int32_t dy)
{
    ball_x += dx;
    if (ball_x < 0 || ball_x > (GFX_W - BALL) * FP) {
        ball_x -= dx;
        vx = -vx;
    }
    else if (break_bricks()) {
        ball_x -= dx;
        vx = -vx;
    }

    int32_t old_bottom = ball_y + BALL * FP;
    ball_y += dy;
    if (ball_y < TOP * FP) {
        ball_y -= dy;
        vy = -vy;
    }
    else if (break_bricks()) {
        ball_y -= dy;
        vy = -vy;
    }
    else if (dy > 0 && old_bottom <= PADDLE_Y * FP && ball_y + BALL * FP > PADDLE_Y * FP) {
        int x = (int) (ball_x / FP);
        if (x + BALL > px && x < px + PADDLE_W) {
            ball_y = (PADDLE_Y - BALL) * FP;
            bounce_on_paddle();
        }
    }
    return ball_y < GFX_H * FP;
}

static void move_ball(void)
{
    int32_t n  = (abs32(vx) > abs32(vy) ? abs32(vx) : abs32(vy)) / FP + 1;
    int32_t sx = vx, sy = vy;
    for (int32_t i = 0; i < n; i++) {
        int32_t dx = sx * (i + 1) / n - sx * i / n;
        int32_t dy = sy * (i + 1) / n - sy * i / n;
        if ((dx != 0 && (sx ^ vx) < 0) || (dy != 0 && (sy ^ vy) < 0)) {
            // It bounced: follow the new direction for the rest of the tick.
            sx = vx;
            sy = vy;
            dx = sx * (i + 1) / n - sx * i / n;
            dy = sy * (i + 1) / n - sy * i / n;
        }
        if (!ball_step(dx, dy)) {
            if (--balls == 0) {
                run_crash(&run);
            }
            else {
                serve();
            }
            return;
        }
        if (wall_cleared()) {
            level++;
            load_wall();
            return;
        }
    }
}

static void move_paddle(int direction)
{
    px += direction * PADDLE_STEP;
    if (px < 0) {
        px = 0;
    }
    if (px > GFX_W - PADDLE_W) {
        px = GFX_W - PADDLE_W;
    }
}

// ---------------------------------------------------------------------------------------------
// The app
// ---------------------------------------------------------------------------------------------

void bricks_enter(void)
{
    run_enter(&run, BEST_BRICKS);
}

void bricks_suspend(void)
{
    run_suspend(&run);
}

void bricks_event(event_t event, uint8_t buttons)
{
    switch (run_event(&run, event)) {
        case RUN_START:
            start_play();
            break;
        case RUN_INPUT:
            if (event == EV_DOWN && buttons != BTN_BOTH && input_held() != BTN_BOTH) {
                move_paddle((buttons & BTN_RIGHT) ? 1 : -1);
                stepped = true;
                if (serving) {
                    ball_x = (px + PADDLE_W / 2 - BALL / 2) * FP;
                }
                ui_dirty();
            }
            break;
        default:
            break;
    }
}

void bricks_tick(void)
{
    if (!run_tick(&run)) {
        return;
    }
    if (!stepped) {
        uint8_t held = input_held();
        move_paddle(((held & BTN_RIGHT) ? 1 : 0) - ((held & BTN_LEFT) ? 1 : 0));
    }
    stepped = false;

    if (serving) {
        ball_x = (px + PADDLE_W / 2 - BALL / 2) * FP;
        if (--serving == 0) {
            launch(3 + (int) random_range(0, 2));  // up, a little left or right
        }
    }
    else {
        move_ball();
    }
    ui_dirty();
}

static void draw_field(void)
{
    char line[16];

    ui_page();
    ui_number(line, level, NULL);
    int x = gfx_text(0, 0, "LEVEL ", 1, true);
    x     = gfx_text(x, 0, line, 1, true);
    for (int i = 1; i < balls; i++) {  // the balls in reserve
        gfx_fill_rect(x + 2 + i * 5, 2, 3, 3, true);
    }
    ui_number(line, run.score, NULL);
    gfx_text_right(GFX_W, 0, line, 1, true);
    ui_rule(8);

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (bricks[r] & (1u << (COLS - 1 - c))) {
                gfx_fill_rect(WALL_X + c * PITCH_X, WALL_Y + r * PITCH_Y, BRICK_W, BRICK_H, true);
            }
        }
    }
    gfx_fill_rect(px, PADDLE_Y, PADDLE_W, PADDLE_H, true);
    gfx_fill_rect((int) (ball_x / FP), (int) (ball_y / FP), BALL, BALL, true);

    if (serving && run.mode == RUN_PLAY) {
        ui_number(line, level, NULL);
        x = (GFX_W - gfx_text_width("LEVEL ", 1) - gfx_text_width(line, 1)) / 2;
        x = gfx_text(x, 42, "LEVEL ", 1, true);
        gfx_text(x, 42, line, 1, true);
    }
}

void bricks_draw(void)
{
    switch (run.mode) {
        case RUN_TITLE:
            run_draw_title(&run, "BRICKS", "HOLD " G_LEFT " OR " G_RIGHT " TO MOVE", NULL);
            break;
        case RUN_OVER:
            run_draw_over(&run, "BRICKS", "SCORE", NULL);
            break;
        default:
            draw_field();
            if (run.mode == RUN_PAUSED) {
                run_draw_paused();
            }
            break;
    }
}
