// Dice (BadgeOS apps/dice): throw one to five dice and read the total.
//
//   BOTH    rolls. The dice tumble for ROLL_TICKS, then settle on the result, which is drawn from
//           the hardware random generator when the roll starts.
//   LEFT    one die fewer, RIGHT one die more (1 to MAX_DICE).
//   HOLD    back to the launcher.
//
// Before the first roll, and after the number of dice changed, the faces 1, 2, 3, ... are drawn
// faint (single-pixel pips), so that nobody reads them as a result.

#include "arcade.h"

#define SIDES      6
#define MAX_DICE   5
#define START_DICE 2
#define ROLL_TICKS 6  // 600 ms of tumbling

#define DIE_MAX  22
#define DIE_GAP  4
#define DICE_Y   12
#define AMOUNT_Y 38

// Where each face puts its pips on a 3 x 3 grid: {column, row}, 1..3.
static const uint8_t PIPS[SIDES][SIDES][2] = {
    {{2, 2}},
    {{1, 1}, {3, 3}},
    {{1, 1}, {2, 2}, {3, 3}},
    {{1, 1}, {3, 1}, {1, 3}, {3, 3}},
    {{1, 1}, {3, 1}, {2, 2}, {1, 3}, {3, 3}},
    {{1, 1}, {3, 1}, {1, 2}, {3, 2}, {1, 3}, {3, 3}},
};

static uint8_t  count;
static uint8_t  faces[MAX_DICE];    // what each die shows: 1..6
static uint8_t  result[MAX_DICE];   // the roll that is tumbling
static bool     settled;            // the faces are a result, not a placeholder
static uint8_t  rolling;            // ticks of tumbling left; 0 when still
static uint16_t total;

static void placeholder(void)
{
    for (uint8_t i = 0; i < count; i++) {
        faces[i] = (uint8_t) (i % SIDES + 1);
    }
    settled = false;
    ui_dirty();
}

static void settle(void)
{
    total = 0;
    for (uint8_t i = 0; i < count; i++) {
        faces[i] = result[i];
        total    = (uint16_t) (total + result[i]);
    }
    settled = true;
    rolling = 0;
    ui_dirty();
}

static void start_roll(void)
{
    if (rolling) {
        return;  // one roll at a time
    }
    for (uint8_t i = 0; i < count; i++) {
        result[i] = (uint8_t) random_range(1, SIDES + 1);
    }
    rolling = ROLL_TICKS;
    ui_dirty();
}

static void change_count(int step)
{
    int to = count + step;
    if (rolling || to < 1 || to > MAX_DICE) {
        return;
    }
    count = (uint8_t) to;
    placeholder();
}

void dice_enter(void)
{
    if (count == 0) {
        count = START_DICE;  // the first time; afterwards the last count is kept
    }
    rolling = 0;
    placeholder();
}

void dice_event(event_t event, uint8_t buttons)
{
    (void) buttons;
    switch (event) {
        case EV_SELECT:
            start_roll();
            break;
        case EV_LEFT:
            change_count(-1);
            break;
        case EV_RIGHT:
            change_count(1);
            break;
        case EV_BACK:
            shell_back();
            break;
        default:
            break;
    }
}

void dice_tick(void)
{
    if (!rolling) {
        return;
    }
    if (--rolling == 0) {
        settle();
        return;
    }
    // The faces shown while the dice tumble are decoration.
    for (uint8_t i = 0; i < count; i++) {
        faces[i] = (uint8_t) random_range(1, SIDES + 1);
    }
    ui_dirty();
}

static int die_size(void)
{
    int room = GFX_W - 8 - (count - 1) * DIE_GAP;
    int size = room / count;
    return size > DIE_MAX ? DIE_MAX : size;
}

static void draw_die(int x, int y, int size, uint8_t face, bool faint)
{
    gfx_round_rect(x, y, size, size, true);
    int pip = faint ? 1 : (size >= 18 ? 3 : 2);
    for (int i = 0; i < face; i++) {
        int cx = x + size * PIPS[face - 1][i][0] / 4;
        int cy = y + size * PIPS[face - 1][i][1] / 4;
        gfx_fill_rect(cx - pip / 2, cy - pip / 2, pip, pip, true);
    }
}

void dice_draw(void)
{
    char line[16];

    ui_page();
    ui_header(UI_HEADER, "DICE");

    int size  = die_size();
    int width = count * size + (count - 1) * DIE_GAP;
    int x     = (GFX_W - width) / 2;
    for (uint8_t i = 0; i < count; i++) {
        draw_die(x + i * (size + DIE_GAP), DICE_Y, size, faces[i], !settled && !rolling);
    }

    // The total, as BadgeOS's amount: a label, the value, the unit.
    int amount_y = DICE_Y + size + 4;
    if (amount_y < AMOUNT_Y) {
        amount_y = AMOUNT_Y;
    }
    gfx_text(0, amount_y + 4, "TOTAL", 1, true);
    if (rolling) {
        gfx_text_center(GFX_W / 2, amount_y, "..", 2, true);
    }
    else if (settled) {
        ui_number(line, total, NULL);
        gfx_text_center(GFX_W / 2, amount_y, line, 2, true);
    }
    else {
        gfx_text_center(GFX_W / 2, amount_y, "--", 2, true);
    }
    ui_number(line, count, count == 1 ? " DIE" : " DICE");
    gfx_text_right(GFX_W, amount_y + 4, line, 1, true);

    ui_footer(G_LEFT "-1", "+1" G_RIGHT);
    gfx_text_center(GFX_W / 2, 57, "BOTH:ROLL", 1, true);
}
