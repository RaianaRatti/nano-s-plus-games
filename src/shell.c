// The shell: the launcher, and the switch that hands each event to the app on screen.
//
// The launcher is BadgeOS's "games" folder as a Nano carousel: LEFT and RIGHT turn through the
// games, BOTH opens one, HOLD (or the last card, Quit) goes back to the Ledger dashboard.

#include <string.h>

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
            return "DODGE";
        case APP_DICE:
            return "DICE";
        case APP_DUEL:
            return "DUEL";
        case APP_SNAKE:
            return "SNAKE";
        case APP_PONG:
            return "PONG";
        case APP_BRICKS:
            return "BRICKS";
        case APP_RACER:
            return "RACER";
        case APP_CAVE:
            return "CAVE";
        case APP_FLIP:
            return "FLIP";
        case APP_RUNNER:
            return "RUNNER";
        default:
            return "QUIT";
    }
}

static const char *card_line(uint8_t i)
{
    switch (APP_DODGE + i) {
        case APP_DODGE:
            return "DODGE THE BLOCKS";
        case APP_DICE:
            return "ROLL ONE TO FIVE DICE";
        case APP_DUEL:
            return "TWO PLAYER REFLEXES";
        case APP_SNAKE:
            return "EAT AND GROW";
        case APP_PONG:
            return "ONE OR TWO PLAYERS";
        case APP_BRICKS:
            return "BREAK EVERY BRICK";
        case APP_RACER:
            return "WEAVE THROUGH TRAFFIC";
        case APP_CAVE:
            return "FLY THROUGH THE CAVE";
        case APP_FLIP:
            return "FLIP THE GRAVITY";
        case APP_RUNNER:
            return "JUMP AND DUCK";
        default:
            return "BACK TO THE DASHBOARD";
    }
}

// The card's best score: its slot and unit. Returns false for a card without one.
static bool card_best(uint8_t i, best_t *slot, const char **unit)
{
    *unit = NULL;
    switch (APP_DODGE + i) {
        case APP_DODGE:
            *slot = BEST_DODGE;
            *unit = " S";
            return true;
        case APP_SNAKE:
            *slot = BEST_SNAKE;
            return true;
        case APP_BRICKS:
            *slot = BEST_BRICKS;
            return true;
        case APP_RACER:
            *slot = BEST_RACER;
            *unit = " M";
            return true;
        case APP_CAVE:
            *slot = BEST_CAVE;
            *unit = " M";
            return true;
        case APP_FLIP:
            *slot = BEST_FLIP;
            *unit = " M";
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
    char page[16];
    ui_number(page, card + 1, "/");
    ui_number(page + strlen(page), NB_CARDS, NULL);

    ui_page();
    ui_header(UI_HEADER, page);
    gfx_text(1, 21, G_LEFT, 1, true);
    gfx_text_right(GFX_W - 1, 21, G_RIGHT, 1, true);
    gfx_text_center(GFX_W / 2, 15, card_title(card), 2, true);
    gfx_text_center(GFX_W / 2, 34, card_line(card), 1, true);
    best_t      slot;
    const char *unit;
    if (card_best(card, &slot, &unit)) {
        char best[16];
        ui_number(best, storage_best(slot), unit);
        gfx_text(28, 44, "BEST", 1, true);
        gfx_text_right(100, 44, best, 1, true);
    }
    ui_footer("BOTH:OPEN", "HOLD:QUIT");
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
