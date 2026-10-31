// The arcade's look: dark ink on a lit page, lower case, few words.
//
//   y 0     the bar: a game's score (and lives) in small figures
//   y 8     a lace edge under it
//   y 10    the page
//
// Menus and results have no bar: a picture, a word or two, a number.
//
// Drawing is on demand: a change calls ui_dirty() and the shell draws the screen once after the
// event that changed it.

#include "arcade.h"

static bool dirty;

void ui_dirty(void)
{
    dirty = true;
}

bool ui_take_dirty(void)
{
    bool was = dirty;
    dirty    = false;
    return was;
}

void ui_page(void)
{
    gfx_clear(false);
}

// A lace edge: a row of little arcs, two pixels tall.
void ui_rule(int y)
{
    for (int x = 0; x < GFX_W; x++) {
        gfx_pixel(x, x % 4 == 0 ? y : y + 1, true);
    }
}

void ui_bar(const char *left, const char *right)
{
    if (left) {
        gfx_text(1, 0, left, 1, true);
    }
    if (right) {
        gfx_text_right(GFX_W - 1, 0, right, 1, true);
    }
    ui_rule(8);
}

// Words cut out of a rounded ink bubble, centred on cx.
void ui_bubble_text(int cx, int y, const char *text, int scale)
{
    int w = gfx_text_width(text, scale) + 4 * scale + 2;
    int h = FONT_H * scale + 2 * scale + 1;
    gfx_bubble(cx - w / 2, y, w, h, 2, true);
    gfx_text_center(cx, y + scale + 1, text, scale, false);
}

// The pause sign in a bubble, over whatever is on screen.
void ui_pause_sign(void)
{
    gfx_bubble(GFX_W / 2 - 16, 20, 32, 30, 2, false);
    gfx_bubble(GFX_W / 2 - 14, 22, 28, 26, 2, true);
    gfx_fill_rect(GFX_W / 2 - 6, 28, 4, 14, false);
    gfx_fill_rect(GFX_W / 2 + 2, 28, 4, 14, false);
}

// The best score: a star and the number, centred on cx.
void ui_best(int cx, int y, uint32_t best)
{
    char line[16];
    ui_number(line, best, NULL);
    int w = FONT_ADV * 2 + gfx_text_width(line, 1);
    int x = gfx_text(cx - w / 2, y, G_STAR, 1, true);
    gfx_text(x + FONT_ADV, y, line, 1, true);
}

void ui_number(char *out, uint32_t value, const char *suffix)
{
    char digits[11];
    int  n = 0;
    do {
        digits[n++] = (char) ('0' + value % 10);
        value /= 10;
    } while (value && n < 10);

    int i = 0;
    while (n) {
        out[i++] = digits[--n];
    }
    while (suffix && *suffix && i < 15) {
        out[i++] = *suffix++;
    }
    out[i] = '\0';
}
