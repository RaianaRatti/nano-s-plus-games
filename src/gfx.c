// The display: a 128 x 64, 1 bit framebuffer, shown as one full-screen NBGL image.
//
// The OS draws NBGL images on Nano devices column by column, from the rightmost column to the
// leftmost, each column from top to bottom, the first pixel in the high bit of a byte (the format
// the SDK's lib_nbgl/tools/icon2glyph.py packs icons in). The framebuffer is kept in exactly that
// format, so showing a frame is one draw of the image: a column is 8 bytes, pixel (x, y) is bit
// (7 - y % 8) of byte (127 - x) * 8 + y / 8.
//
// A set bit is lit (white on the Nano's black screen).

#include <string.h>

#include "os.h"
#include "nbgl_obj.h"
#include "nbgl_screen.h"

#include "arcade.h"

static uint8_t             fb[GFX_W * GFX_H / 8];
static nbgl_icon_details_t fb_icon;
static nbgl_image_t       *image;
static nbgl_obj_t        **children;

void gfx_init(void)
{
    nbgl_screenTickerConfiguration_t ticker;
    memset(&ticker, 0, sizeof(ticker));
    nbgl_screenSet(&children, 1, &ticker, NULL);

    fb_icon.width  = GFX_W;
    fb_icon.height = GFX_H;
    fb_icon.bpp    = NBGL_BPP_1;
    fb_icon.isFile = false;
    fb_icon.bitmap = fb;

    image                       = (nbgl_image_t *) nbgl_objPoolGet(IMAGE, 0);
    image->obj.area.width       = GFX_W;
    image->obj.area.height      = GFX_H;
    image->obj.area.bpp         = NBGL_BPP_1;
    image->obj.area.backgroundColor = BLACK;
    image->obj.alignment        = TOP_LEFT;
    image->obj.alignmentMarginX = 0;
    image->obj.alignmentMarginY = 0;
    image->foregroundColor      = WHITE;
    image->transformation       = NO_TRANSFORMATION;
    image->buffer               = &fb_icon;
    children[0]                 = (nbgl_obj_t *) image;

    memset(fb, 0, sizeof(fb));
}

// Draws the screen again: its background, then the framebuffer image over all of it. (Drawing the
// image object alone does not reach the display until the screen has laid it out.)
void gfx_present(void)
{
    nbgl_screenRedraw();
}

void gfx_clear(bool on)
{
    memset(fb, on ? 0xFF : 0x00, sizeof(fb));
}

void gfx_pixel(int x, int y, bool on)
{
    if (x < 0 || x >= GFX_W || y < 0 || y >= GFX_H) {
        return;
    }
    uint8_t *byte = &fb[(GFX_W - 1 - x) * (GFX_H / 8) + y / 8];
    uint8_t  bit  = 0x80 >> (y % 8);
    if (on) {
        *byte |= bit;
    }
    else {
        *byte &= ~bit;
    }
}

void gfx_fill_rect(int x, int y, int w, int h, bool on)
{
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            gfx_pixel(x + i, y + j, on);
        }
    }
}

void gfx_rect(int x, int y, int w, int h, bool on)
{
    gfx_fill_rect(x, y, w, 1, on);
    gfx_fill_rect(x, y + h - 1, w, 1, on);
    gfx_fill_rect(x, y, 1, h, on);
    gfx_fill_rect(x + w - 1, y, 1, h, on);
}

// A rectangle with its corner pixels cut, for the faces of the dice.
void gfx_round_rect(int x, int y, int w, int h, bool on)
{
    gfx_fill_rect(x + 1, y, w - 2, 1, on);
    gfx_fill_rect(x + 1, y + h - 1, w - 2, 1, on);
    gfx_fill_rect(x, y + 1, 1, h - 2, on);
    gfx_fill_rect(x + w - 1, y + 1, 1, h - 2, on);
}

// Every other pixel: the perforation of the receipt.
void gfx_dotted_hline(int x, int y, int w, bool on)
{
    for (int i = 0; i < w; i += 2) {
        gfx_pixel(x + i, y, on);
    }
}

// ---------------------------------------------------------------------------------------------
// Text: the classic 5 x 7 font, upper case. Each glyph is 5 columns, bit 0 at the top.
// ---------------------------------------------------------------------------------------------

static const uint8_t FONT[][FONT_W] = {
    {0x00, 0x00, 0x00, 0x00, 0x00},  // ' '
    {0x00, 0x00, 0x5F, 0x00, 0x00},  // !
    {0x00, 0x07, 0x00, 0x07, 0x00},  // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14},  // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12},  // $
    {0x23, 0x13, 0x08, 0x64, 0x62},  // %
    {0x36, 0x49, 0x55, 0x22, 0x50},  // &
    {0x00, 0x05, 0x03, 0x00, 0x00},  // '
    {0x00, 0x1C, 0x22, 0x41, 0x00},  // (
    {0x00, 0x41, 0x22, 0x1C, 0x00},  // )
    {0x08, 0x2A, 0x1C, 0x2A, 0x08},  // *
    {0x08, 0x08, 0x3E, 0x08, 0x08},  // +
    {0x00, 0x50, 0x30, 0x00, 0x00},  // ,
    {0x08, 0x08, 0x08, 0x08, 0x08},  // -
    {0x00, 0x60, 0x60, 0x00, 0x00},  // .
    {0x20, 0x10, 0x08, 0x04, 0x02},  // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E},  // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00},  // 1
    {0x42, 0x61, 0x51, 0x49, 0x46},  // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31},  // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10},  // 4
    {0x27, 0x45, 0x45, 0x45, 0x39},  // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30},  // 6
    {0x01, 0x71, 0x09, 0x05, 0x03},  // 7
    {0x36, 0x49, 0x49, 0x49, 0x36},  // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E},  // 9
    {0x00, 0x36, 0x36, 0x00, 0x00},  // :
    {0x00, 0x56, 0x36, 0x00, 0x00},  // ;
    {0x08, 0x14, 0x22, 0x41, 0x00},  // <
    {0x14, 0x14, 0x14, 0x14, 0x14},  // =
    {0x00, 0x41, 0x22, 0x14, 0x08},  // >
    {0x02, 0x01, 0x51, 0x09, 0x06},  // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E},  // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E},  // A
    {0x7F, 0x49, 0x49, 0x49, 0x36},  // B
    {0x3E, 0x41, 0x41, 0x41, 0x22},  // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C},  // D
    {0x7F, 0x49, 0x49, 0x49, 0x41},  // E
    {0x7F, 0x09, 0x09, 0x09, 0x01},  // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A},  // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F},  // H
    {0x00, 0x41, 0x7F, 0x41, 0x00},  // I
    {0x20, 0x40, 0x41, 0x3F, 0x01},  // J
    {0x7F, 0x08, 0x14, 0x22, 0x41},  // K
    {0x7F, 0x40, 0x40, 0x40, 0x40},  // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F},  // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F},  // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E},  // O
    {0x7F, 0x09, 0x09, 0x09, 0x06},  // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E},  // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46},  // R
    {0x46, 0x49, 0x49, 0x49, 0x31},  // S
    {0x01, 0x01, 0x7F, 0x01, 0x01},  // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F},  // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F},  // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F},  // W
    {0x63, 0x14, 0x08, 0x14, 0x63},  // X
    {0x07, 0x08, 0x70, 0x08, 0x07},  // Y
    {0x61, 0x51, 0x49, 0x45, 0x43},  // Z
    {0x00, 0x7F, 0x41, 0x41, 0x00},  // [
    {0x02, 0x04, 0x08, 0x10, 0x20},  // backslash
    {0x00, 0x41, 0x41, 0x7F, 0x00},  // ]
    {0x04, 0x02, 0x01, 0x02, 0x04},  // ^
    {0x40, 0x40, 0x40, 0x40, 0x40},  // _
};

static const uint8_t GLYPHS[][FONT_W] = {
    {0x08, 0x1C, 0x3E, 0x7F, 0x00},  // G_LEFT
    {0x00, 0x7F, 0x3E, 0x1C, 0x08},  // G_RIGHT
    {0x00, 0x00, 0x08, 0x00, 0x00},  // G_DOT
};

static const uint8_t *glyph(char c)
{
    if (c >= 1 && c <= 3) {
        return GLYPHS[c - 1];
    }
    if (c >= 'a' && c <= 'z') {
        c = (char) (c - 'a' + 'A');
    }
    if (c < ' ' || c > '_') {
        c = '?';
    }
    return FONT[c - ' '];
}

int gfx_text_width(const char *text, int scale)
{
    int n = (int) strlen(text);
    if (n == 0) {
        return 0;
    }
    return (n * FONT_ADV - 1) * scale;
}

// Draws text with its top left corner at (x, y). Returns the x after the text.
int gfx_text(int x, int y, const char *text, int scale, bool on)
{
    for (; *text; text++) {
        const uint8_t *g = glyph(*text);
        for (int col = 0; col < FONT_W; col++) {
            for (int row = 0; row < FONT_H; row++) {
                if (g[col] & (1 << row)) {
                    gfx_fill_rect(x + col * scale, y + row * scale, scale, scale, on);
                }
            }
        }
        x += FONT_ADV * scale;
    }
    return x;
}

void gfx_text_center(int cx, int y, const char *text, int scale, bool on)
{
    gfx_text(cx - gfx_text_width(text, scale) / 2, y, text, scale, on);
}

void gfx_text_right(int right, int y, const char *text, int scale, bool on)
{
    gfx_text(right - gfx_text_width(text, scale), y, text, scale, on);
}

// ---------------------------------------------------------------------------------------------
// Sprites
// ---------------------------------------------------------------------------------------------

void gfx_sprite(int x, int y, const uint16_t *rows, int w, int h, bool flip, bool on)
{
    for (int row = 0; row < h; row++) {
        uint16_t bits = rows[flip ? h - 1 - row : row];
        for (int col = 0; col < w; col++) {
            if (bits & (1u << (w - 1 - col))) {
                gfx_pixel(x + col, y + row, on);
            }
        }
    }
}
