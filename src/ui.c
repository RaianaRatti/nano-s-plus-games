// The receipt look of BadgeOS (lib/vk.lua's ui kit), shrunk to 128 x 64:
//
//   y 0     header: BADGEOS on the left, the screen's own words on the right
//   y 8     a perforated rule
//   y 10    the page
//   y 54    a perforated rule
//   y 57    footer: what the buttons do
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

void ui_rule(int y)
{
    gfx_dotted_hline(0, y, GFX_W, true);
}

void ui_header(const char *left, const char *right)
{
    gfx_text(0, 0, left, 1, true);
    if (right) {
        gfx_text_right(GFX_W, 0, right, 1, true);
    }
    ui_rule(8);
}

void ui_footer(const char *left, const char *right)
{
    ui_rule(UI_BOTTOM);
    if (left) {
        gfx_text(0, 57, left, 1, true);
    }
    if (right) {
        gfx_text_right(GFX_W, 57, right, 1, true);
    }
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
