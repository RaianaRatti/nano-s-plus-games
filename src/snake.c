// Snake: the snake crawls on its own, LEFT and RIGHT turn it (its own left and right), food makes
// it longer and faster. The walls and its own body end the run. The score is the food eaten.
//
// The field is a grid of 4 x 4 cells inside a frame. Each cell of the body remembers the direction
// to the next one toward the head, so the tail can follow the body without a list of positions,
// and a cell is taken exactly when it holds a direction.
//
// A press turns at the next step; two quick presses are both kept, for a U-turn. Pressing both
// pauses: the first button of the two has queued a turn by the time the second goes down, and that
// turn is taken back.

#include "arcade.h"

#define CELL    4
#define COLS    31
#define ROWS    13
#define ORIGIN_X 2   // the first cell's top left corner
#define ORIGIN_Y 11

#define START_LENGTH 3
#define GROW         2  // cells gained per food

// Speed in hundredths of a step per tick: 5 steps a second at first, one step a tick at most.
#define SPEED_START 50
#define SPEED_GAIN  3
#define SPEED_MAX   100

#define MAX_TURNS 2

enum { RIGHT, DOWN, LEFT, UP };

static const int8_t DX[4] = {1, 0, -1, 0};
static const int8_t DY[4] = {0, 1, 0, -1};

static run_t    run;
static uint8_t  grid[COLS * ROWS];  // 0: empty; 1 + the direction to the next cell of the body
static uint8_t  head_x, head_y, tail_x, tail_y;
static uint8_t  dir;
static int8_t   turns[MAX_TURNS];   // +1 right, -1 left, oldest first
static uint8_t  nb_turns;
static bool     last_turn_waiting;  // the newest turn has not been taken yet
static uint8_t  grow;
static uint8_t  food_x, food_y;
static uint8_t  speed;
static uint8_t  progress;

static uint8_t *cell(int x, int y)
{
    return &grid[y * COLS + x];
}

static void place_food(void)
{
    // A uniform choice among the free cells.
    uint16_t free_cells = 0;
    for (uint16_t i = 0; i < COLS * ROWS; i++) {
        free_cells += grid[i] == 0;
    }
    if (free_cells == 0) {
        food_x = 0xFF;  // the snake fills the field
        return;
    }
    uint16_t pick = (uint16_t) random_range(0, free_cells);
    for (uint16_t i = 0; i < COLS * ROWS; i++) {
        if (grid[i] == 0 && pick-- == 0) {
            food_x = (uint8_t) (i % COLS);
            food_y = (uint8_t) (i / COLS);
            return;
        }
    }
}

static void start_play(void)
{
    for (uint16_t i = 0; i < COLS * ROWS; i++) {
        grid[i] = 0;
    }
    dir    = RIGHT;
    tail_x = COLS / 2 - START_LENGTH;
    tail_y = ROWS / 2;
    for (int i = 0; i < START_LENGTH; i++) {
        *cell(tail_x + i, tail_y) = 1 + RIGHT;
    }
    head_x            = (uint8_t) (tail_x + START_LENGTH - 1);
    head_y            = tail_y;
    nb_turns          = 0;
    last_turn_waiting = false;
    grow              = 0;
    speed             = SPEED_START;
    progress          = 0;
    place_food();
}

static void step(void)
{
    if (nb_turns) {
        dir = (uint8_t) ((dir + 4 + turns[0]) % 4);
        turns[0] = turns[1];
        nb_turns--;
        if (nb_turns == 0) {
            last_turn_waiting = false;
        }
    }

    int x = head_x + DX[dir];
    int y = head_y + DY[dir];
    if (x < 0 || x >= COLS || y < 0 || y >= ROWS) {
        run_crash(&run);
        return;
    }

    // The tail moves first, so the head may take the cell it leaves.
    if (grow) {
        grow--;
    }
    else {
        uint8_t d       = (uint8_t) (*cell(tail_x, tail_y) - 1);
        *cell(tail_x, tail_y) = 0;
        tail_x          = (uint8_t) (tail_x + DX[d]);
        tail_y          = (uint8_t) (tail_y + DY[d]);
    }

    if (*cell(x, y)) {
        run_crash(&run);
        return;
    }
    *cell(head_x, head_y) = 1 + dir;
    head_x                = (uint8_t) x;
    head_y                = (uint8_t) y;
    *cell(head_x, head_y) = 1 + dir;  // the head's own direction: it is taken

    if (head_x == food_x && head_y == food_y) {
        run.score++;
        grow += GROW;
        if (speed + SPEED_GAIN <= SPEED_MAX) {
            speed += SPEED_GAIN;
        }
        place_food();
    }
}

static void turn(int8_t way)
{
    if (nb_turns < MAX_TURNS) {
        turns[nb_turns++] = way;
        last_turn_waiting = true;
    }
}

// ---------------------------------------------------------------------------------------------
// The app
// ---------------------------------------------------------------------------------------------

void snake_enter(void)
{
    run_enter(&run, BEST_SNAKE);
    start_play();
}

void snake_suspend(void)
{
    run_suspend(&run);
}

void snake_event(event_t event, uint8_t buttons)
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
                // The second button of a pause: take back the first one's turn.
                if (last_turn_waiting && nb_turns) {
                    nb_turns--;
                    last_turn_waiting = false;
                }
            }
            else {
                turn((buttons & BTN_RIGHT) ? 1 : -1);
            }
            break;
        default:
            break;
    }
}

void snake_tick(void)
{
    if (!run_tick(&run)) {
        return;
    }
    progress = (uint8_t) (progress + speed);
    if (progress >= 100) {
        progress = (uint8_t) (progress - 100);
        step();
        ui_dirty();
    }
}

static void draw_field(void)
{
    char score[16];
    ui_number(score, run.score, NULL);

    ui_page();
    ui_bar(NULL, score);
    gfx_rect(0, ORIGIN_Y - 1, GFX_W, GFX_H - ORIGIN_Y + 1, true);

    // The body, tail to head: a 3 x 3 square per cell, joined to the next one.
    uint8_t x = tail_x, y = tail_y;
    for (;;) {
        int px = ORIGIN_X + x * CELL;
        int py = ORIGIN_Y + y * CELL;
        gfx_fill_rect(px, py, CELL - 1, CELL - 1, true);
        if (x == head_x && y == head_y) {
            // The eye, looking the way it goes.
            gfx_pixel(px + 1 + DX[dir], py + 1 + DY[dir], false);
            break;
        }
        uint8_t d = (uint8_t) (*cell(x, y) - 1);
        gfx_fill_rect(px + (d == RIGHT ? CELL - 1 : 0) - (d == LEFT ? 1 : 0),
                      py + (d == DOWN ? CELL - 1 : 0) - (d == UP ? 1 : 0),
                      (d == RIGHT || d == LEFT) ? 1 : CELL - 1,
                      (d == DOWN || d == UP) ? 1 : CELL - 1,
                      true);
        x = (uint8_t) (x + DX[d]);
        y = (uint8_t) (y + DY[d]);
    }

    // The food: a small cross.
    if (food_x != 0xFF) {
        int fx = ORIGIN_X + food_x * CELL;
        int fy = ORIGIN_Y + food_y * CELL;
        gfx_fill_rect(fx + 1, fy, 1, 3, true);
        gfx_fill_rect(fx, fy + 1, 3, 1, true);
    }
}

void snake_draw(void)
{
    if (run.mode == RUN_OVER) {
        run_draw_over(&run);
        return;
    }
    draw_field();
    run_draw_overlay(&run);
}
