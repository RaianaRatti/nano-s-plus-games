// Curio: a pocket gallery of lesser-known works of art, each with its artist and its story.
//
//   the card   one work: its number and short title in the bar, the whole work under it.
//              LEFT and RIGHT turn through the works, BOTH opens one, HOLD quits.
//   the work   its pages: the whole work as big as the screen allows, a closer look at the part
//              that matters (for most works), then about (title, artist, date) and the story. LEFT and RIGHT turn the pages, and past either
//              end go back to the card; BOTH or HOLD go back to it at once.
//
// The pictures (each work's card, then its pages) and the texts are in works.c, made by tools/make_gallery.py from
// gallery/works.json. The work last seen is kept in the app's flash: the gallery opens on it.

#include "os.h"
#include "main_std_app.h"
#include "app_storage.h"

#include "core.h"
#include "works.h"

#define LINE_H     9
#define PAGE_LINES 7
#define LINE_MAX   20  // characters (the generator wraps the texts to it)
#define CARD_Y     UI_TOP
#define BAR_X      126 // the scroll bar of the text pages, 2 pixels wide

#define SAVE_TAG 0xC0  // marks a saved work number

static uint8_t work;
static int8_t  page;   // -1: the card; then the details, about, the story's pages
static uint8_t saved;  // the work number in flash (0xFF: none)

static const work_t *current(void)
{
    return &WORKS[work];
}

static uint8_t story_pages(const work_t *w)
{
    return (uint8_t) ((w->story_lines + PAGE_LINES - 1) / PAGE_LINES);
}

static uint8_t nb_pages(const work_t *w)
{
    return (uint8_t) (w->details + 1 + story_pages(w));
}

// ---------------------------------------------------------------------------------------------
// The work last seen, in flash: a tag and the number, written only when it changed.
// ---------------------------------------------------------------------------------------------

static void load(void)
{
    uint8_t data[2];
    saved = 0xFF;
    if (app_storage_read(data, sizeof(data), 0) == (int32_t) sizeof(data) && data[0] == SAVE_TAG &&
        data[1] < NB_WORKS) {
        saved = data[1];
        work  = data[1];
    }
}

static void save(void)
{
    if (work == saved) {
        return;
    }
    uint8_t data[2] = {SAVE_TAG, work};
    app_storage_write(data, sizeof(data), 0);
    app_storage_increment_data_version();
    saved = work;
}

// ---------------------------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------------------------

// A picture, centred across the screen and in the rows from y, h tall.
static void draw_picture(uint8_t index, int y, int h)
{
    const picture_t *p      = &PICTURES[index];
    const uint8_t   *bits   = &ART_BITS[p->bits];
    int              stride = (p->w + 7) / 8;
    int              x0     = (GFX_W - p->w) / 2;
    int              y0     = y + (h - p->h) / 2;
    for (int row = 0; row < p->h; row++) {
        for (int col = 0; col < p->w; col++) {
            if (bits[row * stride + col / 8] & (0x80 >> (col % 8))) {
                gfx_pixel(x0 + col, y0 + row, true);
            }
        }
    }
}

// Copies line n of a text (lines end with '\n', the text with '\0') into out.
static void text_line(uint16_t text, uint8_t n, char out[LINE_MAX + 1])
{
    const char *c = &ART_TEXT[text];
    while (n && *c) {
        if (*c++ == '\n') {
            n--;
        }
    }
    int i = 0;
    while (*c && *c != '\n' && i < LINE_MAX) {
        out[i++] = *c++;
    }
    out[i] = '\0';
}

static void draw_lines(uint16_t text, uint8_t first, uint8_t count, int y)
{
    char line[LINE_MAX + 1];
    for (uint8_t i = 0; i < count; i++) {
        text_line(text, (uint8_t) (first + i), line);
        gfx_text(1, y + i * LINE_H, line, 1, true);
    }
}

// Where the page is among the work's pages, along the right edge.
static void draw_scroll_bar(const work_t *w)
{
    int n     = nb_pages(w);
    int thumb = GFX_H / n > 4 ? GFX_H / n : 4;
    int y     = (GFX_H - thumb) * page / (n - 1);
    for (int i = 0; i < GFX_H; i += 2) {
        gfx_pixel(BAR_X + 1, i, true);
    }
    gfx_fill_rect(BAR_X, y, 2, thumb, true);
}

static void draw_card(void)
{
    const work_t *w = current();
    char          number[16];
    ui_number(number, work + 1u, "/");
    int n = 0;
    while (number[n]) {
        n++;
    }
    ui_number(number + n, NB_WORKS, NULL);
    ui_bar(number, w->short_title);
    draw_picture(w->picture, CARD_Y, GFX_H - CARD_Y);
}

// The title, a dotted rule, then the artist and the date: centred on the screen.
static void draw_about(const work_t *w)
{
    int height = (w->title_lines + w->about_lines) * LINE_H;
    int y      = (GFX_H - height) / 2;
    draw_lines(w->about, 0, w->title_lines, y);
    y += w->title_lines * LINE_H;
    gfx_dotted_hline(1, y - 1, 60, true);
    draw_lines(w->about, w->title_lines, w->about_lines, y + 1);
}

static void draw_work(void)
{
    const work_t *w = current();
    if (page < w->details) {
        draw_picture((uint8_t) (w->picture + 1 + page), 0, GFX_H);
        return;
    }
    if (page == w->details) {
        draw_about(w);
    }
    else {
        uint8_t first = (uint8_t) ((page - w->details - 1) * PAGE_LINES);
        uint8_t count = w->story_lines - first < PAGE_LINES ? (uint8_t) (w->story_lines - first) : PAGE_LINES;
        draw_lines(w->story, first, count, 1);
    }
    draw_scroll_bar(w);
}

// ---------------------------------------------------------------------------------------------
// The shell
// ---------------------------------------------------------------------------------------------

void shell_init(void)
{
    input_reset();
    load();
    page = -1;
    ui_dirty();
}

void shell_back(void)
{
    save();
    app_exit();
}

void shell_event(event_t event, uint8_t buttons)
{
    (void) buttons;
    const work_t *w = current();
    if (page < 0) {
        switch (event) {
            case EV_LEFT:
                work = (uint8_t) ((work + NB_WORKS - 1) % NB_WORKS);
                break;
            case EV_RIGHT:
                work = (uint8_t) ((work + 1) % NB_WORKS);
                break;
            case EV_SELECT:
                page = 0;
                break;
            case EV_BACK:
                shell_back();
                return;
            default:
                return;
        }
    }
    else {
        switch (event) {
            case EV_LEFT:
                page--;  // from the first page, back to the card
                break;
            case EV_RIGHT:
                page = page + 1 < nb_pages(w) ? (int8_t) (page + 1) : -1;
                break;
            case EV_SELECT:
            case EV_BACK:
                page = -1;
                break;
            default:
                return;
        }
    }
    ui_dirty();
}

void shell_tick(void) {}

// The device locked: remember where we are.
void shell_suspend(void)
{
    save();
}

void shell_render(void)
{
    if (!ui_take_dirty()) {
        return;
    }
    ui_page();
    if (page < 0) {
        draw_card();
    }
    else {
        draw_work();
    }
    gfx_present();
}
