// Pong: a ball, two paddles, first to five points.
//
//   1 PLAYER   you are the left paddle: hold LEFT to move it up, RIGHT to move it down. The
//              right paddle is the device's.
//   2 PLAYERS  one button each, like Duel. Each paddle slides up and down on its own and a press
//              of your button turns it around.
//
// Where the ball meets the paddle sets the angle it leaves at, and every return is a little faster.
// The ball moves in steps of at most one pixel within a tick, so a fast ball cannot pass through a
// paddle.
//
//   Title    LEFT and RIGHT choose the mode, BOTH starts, HOLD goes back to the launcher.
//   Playing  HOLD returns to the title. With one player BOTH pauses; two players press both
//            buttons together during play all the time, so they pause by locking the device.
//   Over     BOTH plays again, HOLD returns to the title.

#include "arcade.h"

#define FP 16

#define TOP     10  // the first row the ball may use
#define BOTTOM  GFX_H
#define PADDLE_W 2
#define PADDLE_H 12
#define LEFT_X   2
#define RIGHT_X  (GFX_W - 2 - PADDLE_W)
#define BALL     2

#define POINTS        5
#define SERVE_TICKS   10
#define RESULT_TICKS  30

// Speeds, in 1/16 pixel per tick.
#define BALL_START  48  // across the court in about 2.7 s
#define BALL_GAIN   3   // per return
#define BALL_MAX    112
#define ANGLE       7   // vertical speed per pixel between the ball and the paddle's centre
#define PLAYER_STEP (4 * FP)
#define SLIDE_STEP  (3 * FP)
#define CPU_STEP    (3 * FP)

#define P_LEFT  0
#define P_RIGHT 1

typedef enum { TITLE, SERVE, PLAY, PAUSED, OVER } pong_mode_t;

static pong_mode_t mode;
static pong_mode_t paused_from;
static bool        two_players;
static uint8_t     points[2];
static int32_t     paddle_y[2];  // the top, 1/16 pixels
static int8_t      slide[2];     // two players: which way each paddle slides, +1 down
static int32_t     ball_x, ball_y, vx, vy;
static uint8_t     wait;
static int8_t      server;       // who the next serve goes toward
static int8_t      cpu_aim;      // the device misjudges the ball by this many pixels
static bool        stepped;

static int32_t clamp(int32_t v, int32_t low, int32_t high)
{
    return v < low ? low : (v > high ? high : v);
}

static int32_t abs32(int32_t v)
{
    return v < 0 ? -v : v;
}

static void move_paddle(int who, int32_t by)
{
    paddle_y[who] = clamp(paddle_y[who] + by, TOP * FP, (BOTTOM - PADDLE_H) * FP);
}

static void serve(void)
{
    ball_x = (GFX_W / 2 - BALL / 2) * FP;
    ball_y = (int32_t) random_range(TOP + 8, BOTTOM - 8 - BALL) * FP;
    vx     = server == P_LEFT ? -BALL_START : BALL_START;
    vy     = (int32_t) random_range(0, 2 * 20 + 1) - 20;
    wait   = SERVE_TICKS;
    mode   = SERVE;
    cpu_aim = (int8_t) ((int) random_range(0, 11) - 5);
}

static void start_match(void)
{
    points[P_LEFT]  = 0;
    points[P_RIGHT] = 0;
    for (int i = 0; i < 2; i++) {
        paddle_y[i] = ((TOP + BOTTOM - PADDLE_H) / 2) * FP;
        slide[i]    = i == P_LEFT ? 1 : -1;
    }
    server  = (int8_t) random_range(0, 2);
    stepped = false;
    serve();
    ui_dirty();
}

static void score(int who)
{
    points[who]++;
    server = who == P_LEFT ? P_RIGHT : P_LEFT;  // the serve goes to whoever lost the point
    if (points[who] >= POINTS) {
        mode = OVER;
        input_swallow();
    }
    else {
        serve();
    }
    ui_dirty();
}

// The ball meets a paddle: back the other way, faster, at an angle set by where it hit.
static void hit(int who)
{
    int32_t speed = abs32(vx) + BALL_GAIN;
    if (speed > BALL_MAX) {
        speed = BALL_MAX;
    }
    vx = who == P_LEFT ? speed : -speed;

    int32_t offset = (ball_y + BALL * FP / 2) - (paddle_y[who] + PADDLE_H * FP / 2);
    vy = offset * ANGLE / FP;
    if (who == P_LEFT) {
        cpu_aim = (int8_t) ((int) random_range(0, 13) - 6);
    }
}

// One pixel (at most) of the ball's path.
static bool ball_step(int32_t dx, int32_t dy)
{
    int32_t old_x = ball_x;
    ball_x += dx;
    ball_y += dy;

    if (ball_y < TOP * FP) {
        ball_y = 2 * TOP * FP - ball_y;
        vy     = -vy;
    }
    if (ball_y > (BOTTOM - BALL) * FP) {
        ball_y = 2 * (BOTTOM - BALL) * FP - ball_y;
        vy     = -vy;
    }

    int top = (int) (ball_y / FP);
    for (int who = 0; who < 2; who++) {
        int  py    = (int) (paddle_y[who] / FP);
        bool level = top + BALL > py && top < py + PADDLE_H;
        if (!level) {
            continue;
        }
        int32_t face_left = (who == P_LEFT ? LEFT_X + PADDLE_W : RIGHT_X - BALL) * FP;
        if (who == P_LEFT && dx < 0 && old_x >= face_left && ball_x < face_left) {
            ball_x = face_left;
            hit(who);
            return false;
        }
        if (who == P_RIGHT && dx > 0 && old_x <= face_left && ball_x > face_left) {
            ball_x = face_left;
            hit(who);
            return false;
        }
    }

    if (ball_x < -BALL * FP) {
        score(P_RIGHT);
        return false;
    }
    if (ball_x > GFX_W * FP) {
        score(P_LEFT);
        return false;
    }
    return true;
}

static void move_ball(void)
{
    int32_t n = (abs32(vx) > abs32(vy) ? abs32(vx) : abs32(vy)) / FP + 1;
    int32_t sx = vx, sy = vy;  // a hit changes the speed: the rest of the tick uses the old one
    for (int32_t i = 0; i < n; i++) {
        int32_t dx = sx * (i + 1) / n - sx * i / n;
        int32_t dy = sy * (i + 1) / n - sy * i / n;
        if (!ball_step(dx, dy)) {
            return;
        }
    }
}

static void move_cpu(void)
{
    int32_t target;
    if (vx > 0) {
        target = ball_y + (BALL / 2 + cpu_aim - PADDLE_H / 2) * FP;
    }
    else {
        target = ((TOP + BOTTOM - PADDLE_H) / 2) * FP;  // back to the middle
    }
    int32_t by = clamp(target - paddle_y[P_RIGHT], -CPU_STEP, CPU_STEP);
    move_paddle(P_RIGHT, by);
}

static void move_paddles(void)
{
    if (two_players) {
        for (int who = 0; who < 2; who++) {
            move_paddle(who, slide[who] * SLIDE_STEP);
            int32_t y = paddle_y[who];
            if (y <= TOP * FP || y >= (BOTTOM - PADDLE_H) * FP) {
                slide[who] = (int8_t) -slide[who];
            }
        }
        return;
    }
    if (!stepped) {
        uint8_t held = input_held();
        int     dir  = ((held & BTN_RIGHT) ? 1 : 0) - ((held & BTN_LEFT) ? 1 : 0);
        move_paddle(P_LEFT, dir * PLAYER_STEP);
    }
    stepped = false;
    move_cpu();
}

// ---------------------------------------------------------------------------------------------
// The app
// ---------------------------------------------------------------------------------------------

void pong_enter(void)
{
    mode = TITLE;
    ui_dirty();
}

void pong_suspend(void)
{
    if (mode == SERVE || mode == PLAY) {
        paused_from = mode;
        mode        = PAUSED;
        ui_dirty();
    }
}

void pong_event(event_t event, uint8_t buttons)
{
    if (event == EV_BACK) {
        if (mode == TITLE) {
            shell_back();
        }
        else {
            mode = TITLE;
            ui_dirty();
        }
        return;
    }

    switch (mode) {
        case TITLE:
            if (event == EV_LEFT || event == EV_RIGHT) {
                two_players = !two_players;
                ui_dirty();
            }
            else if (event == EV_SELECT) {
                start_match();
            }
            break;

        case SERVE:
        case PLAY:
            if (event == EV_SELECT && !two_players) {
                pong_suspend();
            }
            else if (event == EV_DOWN && buttons != BTN_BOTH) {
                if (two_players) {
                    int who     = (buttons & BTN_LEFT) ? P_LEFT : P_RIGHT;
                    slide[who]  = (int8_t) -slide[who];
                }
                else if (input_held() != BTN_BOTH) {
                    move_paddle(P_LEFT, ((buttons & BTN_RIGHT) ? 1 : -1) * PLAYER_STEP);
                    stepped = true;
                }
                ui_dirty();
            }
            break;

        case PAUSED:
            if (event == EV_SELECT) {
                mode = paused_from;
                ui_dirty();
            }
            break;

        case OVER:
            if (event == EV_SELECT) {
                start_match();
            }
            break;
    }
}

void pong_tick(void)
{
    if (mode == SERVE) {
        move_paddles();
        if (--wait == 0) {
            mode = PLAY;
        }
        ui_dirty();
    }
    else if (mode == PLAY) {
        move_paddles();
        move_ball();
        ui_dirty();
    }
}

// ---------------------------------------------------------------------------------------------
// Screens
// ---------------------------------------------------------------------------------------------

static const char *name(int who)
{
    if (two_players) {
        return who == P_LEFT ? "LEFT" : "RIGHT";
    }
    return who == P_LEFT ? "YOU" : "CPU";
}

static void draw_court(void)
{
    char left[16], right[16];
    char digits[16];

    // "YOU 2" on the left, "1 CPU" on the right.
    int n = 0;
    for (const char *p = name(P_LEFT); *p; p++) {
        left[n++] = *p;
    }
    left[n++] = ' ';
    ui_number(left + n, points[P_LEFT], NULL);
    ui_number(digits, points[P_RIGHT], " ");
    n = 0;
    for (const char *p = digits; *p; p++) {
        right[n++] = *p;
    }
    for (const char *p = name(P_RIGHT); *p; p++) {
        right[n++] = *p;
    }
    right[n] = '\0';

    ui_page();
    ui_header(left, right);
    for (int y = TOP + 1; y < BOTTOM; y += 4) {
        gfx_fill_rect(GFX_W / 2 - 1, y, 1, 2, true);
    }
    gfx_fill_rect(LEFT_X, (int) (paddle_y[P_LEFT] / FP), PADDLE_W, PADDLE_H, true);
    gfx_fill_rect(RIGHT_X, (int) (paddle_y[P_RIGHT] / FP), PADDLE_W, PADDLE_H, true);
    if (mode != SERVE || wait % 4 < 2) {  // the ball blinks before the serve
        gfx_fill_rect((int) (ball_x / FP), (int) (ball_y / FP), BALL, BALL, true);
    }
}

static void draw_over(void)
{
    char line[24];
    int  winner = points[P_LEFT] >= POINTS ? P_LEFT : P_RIGHT;

    ui_page();
    ui_header(UI_HEADER, "PONG");
    if (two_players) {
        gfx_text_center(GFX_W / 2, 13, winner == P_LEFT ? "LEFT WINS" : "RIGHT WINS", 2, true);
    }
    else {
        gfx_text_center(GFX_W / 2, 13, winner == P_LEFT ? "YOU WIN" : "CPU WINS", 2, true);
    }
    ui_number(line, points[P_LEFT], " " G_DOT " ");
    int n = 0;
    while (line[n]) {
        n++;
    }
    ui_number(line + n, points[P_RIGHT], NULL);
    gfx_text_center(GFX_W / 2, 35, line, 1, true);
    ui_footer("BOTH:AGAIN", "HOLD:BACK");
}

void pong_draw(void)
{
    switch (mode) {
        case TITLE:
            ui_page();
            ui_header(UI_HEADER, "PONG");
            gfx_text_center(GFX_W / 2, 13, "PONG", 2, true);
            gfx_text(1, 31, G_LEFT, 1, true);
            gfx_text_right(GFX_W - 1, 31, G_RIGHT, 1, true);
            gfx_text_center(GFX_W / 2, 31, two_players ? "2 PLAYERS" : "1 PLAYER VS CPU", 1, true);
            gfx_text_center(GFX_W / 2, 42,
                            two_players ? "PRESS TO TURN AROUND" : G_LEFT " UP  " G_RIGHT " DOWN",
                            1, true);
            ui_footer("BOTH:START", "HOLD:BACK");
            break;

        case SERVE:
        case PLAY:
            draw_court();
            break;

        case PAUSED:
            draw_court();
            gfx_fill_rect(14, 20, GFX_W - 28, 28, false);
            gfx_rect(14, 20, GFX_W - 28, 28, true);
            gfx_text_center(GFX_W / 2, 24, "PAUSED", 1, true);
            gfx_text_center(GFX_W / 2, 33, "BOTH:RESUME", 1, true);
            gfx_text_center(GFX_W / 2, 40, "HOLD:QUIT", 1, true);
            break;

        case OVER:
            draw_over();
            break;
    }
}
