// The physics facts the pet tells: written by tools/make_facts.py into facts.c, from pet/facts/.
//
// Nothing here holds a pointer (a pointer stored in the app's constants would need PIC()): the
// texts are offsets into FACT_TEXT.

#pragma once

#include <stdint.h>

typedef struct {
    uint32_t title;        // offset into FACT_TEXT: the concept, for the bar ('\0')
    uint32_t text;         // offset into FACT_TEXT: lines of up to 20 characters ('\n'), '\0'
    uint8_t  fact_lines;   // the first lines: the fact itself, in the box
    uint8_t  lines;        // all of them: the fact, then what explains it, on the pages
} fact_t;

extern const uint8_t NB_FACTS;
extern const fact_t  FACTS[];
extern const char    FACT_TEXT[];
