// The shell: the launcher, and the switch that hands each event to the app on screen.
//
// The launcher is a carousel of cards, a picture and a name each: LEFT and RIGHT turn through the
// games, BOTH opens one, HOLD (or the last card, bye) goes back to the Ledger dashboard.

#include "os.h"
#include "main_std_app.h"

#include "arcade.h"

static app_id_t current;

// ---------------------------------------------------------------------------------------------
// The launcher
// ---------------------------------------------------------------------------------------------

#define CARD_QUIT NB_GAMES
#define NB_CARDS  (NB_GAMES + 1)

static uint8_t card;

static const char *card_title(uint8_t i)
{
    switch (APP_DODGE + i) {
        case APP_DODGE:
            return "dodge";
        case APP_DICE:
            return "dice";
        case APP_DUEL:
            return "duel";
        case APP_SNAKE:
            return "snake";
        case APP_PONG:
            return "pong";
        case APP_BRICKS:
            return "bricks";
        case APP_RACER:
            return "racer";
        case APP_CAVE:
            return "cave";
        case APP_FLIP:
            return "flip";
        case APP_RUNNER:
            return "runner";
        default:
            return "bye!";
    }
}

// The card's best score. Returns false for a card without one.
static bool card_best(uint8_t i, best_t *slot)
{
    switch (APP_DODGE + i) {
        case APP_DODGE:
            *slot = BEST_DODGE;
            return true;
        case APP_SNAKE:
            *slot = BEST_SNAKE;
            return true;
        case APP_BRICKS:
            *slot = BEST_BRICKS;
            return true;
        case APP_RACER:
            *slot = BEST_RACER;
            return true;
        case APP_CAVE:
            *slot = BEST_CAVE;
            return true;
        case APP_FLIP:
            *slot = BEST_FLIP;
            return true;
        case APP_RUNNER:
            *slot = BEST_RUNNER;
            return true;
        default:
            return false;
    }
}

void launcher_enter(void)
{
    ui_dirty();
}

void launcher_event(event_t event, uint8_t buttons)
{
    (void) buttons;
    switch (event) {
        case EV_LEFT:
            card = (uint8_t) ((card + NB_CARDS - 1) % NB_CARDS);
            ui_dirty();
            break;
        case EV_RIGHT:
            card = (uint8_t) ((card + 1) % NB_CARDS);
            ui_dirty();
            break;
        case EV_SELECT:
            if (card == CARD_QUIT) {
                app_exit();
            }
            shell_open((app_id_t) (APP_DODGE + card));
            break;
        case EV_BACK:
            app_exit();
            break;
        default:
            break;
    }
}

void launcher_draw(void)
{
    ui_page();

    ui_icon(card, 6, 10, 2);
    gfx_text_center(88, 15, card_title(card), 2, true);
    best_t slot;
    if (card_best(card, &slot)) {
        ui_best(88, 36, storage_best(slot));
    }

    // A dot per card, the current one bigger.
    int x0 = GFX_W / 2 - (NB_CARDS - 1) * 3;
    for (int i = 0; i < NB_CARDS; i++) {
        if (i == card) {
            gfx_bubble(x0 + i * 6 - 2, 54, 5, 5, 1, true);
        }
        else {
            gfx_pixel(x0 + i * 6, 56, true);
        }
    }
}

// ---------------------------------------------------------------------------------------------
// The switch
// ---------------------------------------------------------------------------------------------

void shell_init(void)
{
    input_reset();
    current = APP_LAUNCHER;
    launcher_enter();
}

// One case per app, calling the app's function of that name. (A table of function pointers would
// need PIC() on every call on this device; a switch does not.)
#define DISPATCH(fn, ...)                    \
    switch (current) {                       \
        case APP_DODGE:                      \
            dodge_##fn(__VA_ARGS__);         \
            break;                           \
        case APP_DICE:                       \
            dice_##fn(__VA_ARGS__);          \
            break;                           \
        case APP_DUEL:                       \
            duel_##fn(__VA_ARGS__);          \
            break;                           \
        case APP_SNAKE:                      \
            snake_##fn(__VA_ARGS__);         \
            break;                           \
        case APP_PONG:                       \
            pong_##fn(__VA_ARGS__);          \
            break;                           \
        case APP_BRICKS:                     \
            bricks_##fn(__VA_ARGS__);        \
            break;                           \
        case APP_RACER:                      \
            racer_##fn(__VA_ARGS__);         \
            break;                           \
        case APP_CAVE:                       \
            cave_##fn(__VA_ARGS__);          \
            break;                           \
        case APP_FLIP:                       \
            flip_##fn(__VA_ARGS__);          \
            break;                           \
        case APP_RUNNER:                     \
            runner_##fn(__VA_ARGS__);        \
            break;                           \
        default:                             \
            launcher_##fn(__VA_ARGS__);      \
            break;                           \
    }

// Dice and Duel stand still when the device locks, and the launcher always does.
static void dice_suspend(void) {}
static void duel_suspend(void) {}
static void launcher_tick(void) {}
static void launcher_suspend(void) {}

void shell_open(app_id_t app)
{
    current = app;
    DISPATCH(enter)
    ui_dirty();
}

void shell_back(void)
{
    shell_open(APP_LAUNCHER);
}

void shell_event(event_t event, uint8_t buttons)
{
    DISPATCH(event, event, buttons)
}

void shell_tick(void)
{
    DISPATCH(tick)
}

// The OS took the screen (the device locked): a game in motion pauses.
void shell_suspend(void)
{
    DISPATCH(suspend)
}

void shell_render(void)
{
    if (!ui_take_dirty()) {
        return;
    }
    DISPATCH(draw)
    gfx_present();
}
