// The gallery's works: written by tools/make_gallery.py into works.c, from gallery/works.json.
//
// Nothing here holds a pointer (a pointer stored in the app's constants would need PIC()): the
// pictures are offsets into ART_BITS, the texts offsets into ART_TEXT.

#pragma once

#include <stdint.h>

// A 1-bit picture: rows of (w + 7) / 8 bytes, the leftmost pixel in the high bit, 1 for ink.
typedef struct {
    uint32_t bits;  // offset into ART_BITS
    uint8_t  w, h;
} picture_t;

typedef struct {
    char     short_title[16];  // for the bar
    uint16_t about;            // offset into ART_TEXT: the title's lines, then the others ('\n')
    uint8_t  title_lines;
    uint8_t  about_lines;      // after the title
    uint16_t story;            // offset into ART_TEXT: lines of up to 20 characters ('\n')
    uint8_t  story_lines;
    uint8_t  picture;          // PICTURES[picture] is the card, its pages' pictures follow it
    uint8_t  details;          // pictures on pages: the whole work, then the closer looks
} work_t;

extern const uint8_t   NB_WORKS;
extern const work_t    WORKS[];
extern const picture_t PICTURES[];
extern const uint8_t   ART_BITS[];
extern const char      ART_TEXT[];
