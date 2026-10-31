// Pet: a little creature to raise, from an egg to a grown-up that sets off into the world.
//
// The room:
//   the bar     its level, and three meters of four pips: food, fun and energy
//   the room    it wanders about; when it needs something, a bubble shows the action to pick
//   the shelf   seven actions: LEFT and RIGHT choose one, BOTH does it, HOLD quits
//
//     meal    fills food                    treat   a little food and a lot of fun, but three
//     play    a game of catch (below)               in a row (without a meal) upset its tummy
//     clean   the floor                     sleep   lights off, or on again
//     meds    cures it when it is sick      info    its progress, and the collection
//
// Every action it takes well (a meal, a treat, a game, a clean, meds, lights off or on) is
// followed by a physics fact: once its scene is over, it says one in a box over the room. BOTH
// opens the box on pages that explain it (LEFT and RIGHT turn them); LEFT or RIGHT closes it. Its
// life waits while the fact is on screen. The facts are in facts.c, and the pet goes through all
// of them before it tells one again.
//
// Good care makes it grow. Every half minute it spends fed, happy, rested, clean and well is a
// point of experience, and so is every good thing done for it. Levels 1-2 are a baby, 3-5 a kid,
// 6-7 a teen and 8-10 a grown-up, whose form depends on how it was raised until then:
//
//     grump   neglected: five mistakes or more
//     chonk   spoiled: at least as many treats as meals (and eight or more)
//     zoomie  played a lot: fifty catches or more
//     angel   perfect care: no mistake at all
//     fluff   anything else: a few mistakes
//
// A mistake is a meter running empty, falling asleep from exhaustion, or falling sick, which
// comes from a minute of hunger, a minute with poop on the floor, or too many treats. A pet left
// sick for three minutes runs away. When the grown-up completes level 10 it says goodbye, its form
// gets a star in the collection and a new egg arrives: the goal is to collect all five.
//
// Catch: five balls are thrown from the left or the right. Press that side's button just as the
// ball reaches the pet: too soon, or the wrong side, and it drops. Older pets throw faster.
//
// The device has no clock, so the pet lives only while it is on screen: away from it, the pet
// waits exactly as it was left. It is saved in the app's flash when you leave (HOLD in the room
// quits the app), when the device locks, every two minutes, and at every step of its life.

#include <string.h>

#include "pet.h"
#include "facts.h"

#define SECOND 10  // ticks

#define LOW  25  // a meter under this is a need
#define FULL 100

// The meters lose a point every so many seconds (twice as slowly asleep, fun twice as fast sick).
#define FOOD_SECS   5
#define FUN_SECS    4
#define ENERGY_SECS 6
#define REST_GAIN   2    // energy won per second asleep

#define CARE_SECS   30   // a point of experience per half minute of good care
#define SICK_SECS   60   // a minute hungry, or with poop on the floor, makes it sick
#define AWAY_SECS   180  // three minutes sick and it runs away
#define DIRTY_SECS  15   // poop on the floor for this long: it asks for a clean
#define SAVE_SECS   120
#define DAY_SECS    240  // a pet day, on the info page
#define HATCH       20   // seconds in the egg; a tap counts as one more

#define MAX_LEVEL  10
#define MAX_POOPS  3
#define TREAT_ACHE 3
#define MEAL_FOOD  30
#define TREAT_FOOD 10
#define TREAT_FUN  20
#define PLAY_ENERGY 15
#define PLAY_FOOD   5
#define CATCH_FUN   8   // per ball caught

#define PET_VERSION 1

enum { STAGE_EGG, STAGE_BABY, STAGE_KID, STAGE_TEEN, STAGE_ADULT };

// The grown-up forms, in the order of the collection.
enum { FORM_FLUFF, FORM_ZOOMIE, FORM_CHONK, FORM_ANGEL, FORM_GRUMP, NB_FORMS };

// How to raise each one, on the info page. (Words in arrays, not pointers: a pointer stored in the
// app's constants would need PIC().)
static const char HINTS[NB_FORMS][22] = {
    "a few mistakes", "plays a lot", "more cake than meals", "never a mistake", "many mistakes",
};

// The bodies: the young ones, then one per form.
enum { BODY_BABY, BODY_KID, BODY_TEEN, BODY_ADULT };

// The shelf.
enum { DO_MEAL, DO_TREAT, DO_PLAY, DO_CLEAN, DO_SLEEP, DO_MEDS, DO_INFO, NB_DOS, DO_NOTHING = 0xFF };

// The little scenes the room plays after an action.
enum { SHOW_NONE, SHOW_EAT, SHOW_HAPPY, SHOW_NO, SHOW_CLEAN, SHOW_MEDS, SHOW_LEVEL, SHOW_GROW, SHOW_HATCH };

enum { MODE_ROOM, MODE_INFO, MODE_CATCH, MODE_BYE, MODE_FACT, MODE_READ };

enum { FACE_OK, FACE_HAPPY, FACE_SAD, FACE_SLEEP, FACE_SICK, FACE_NO, FACE_EAT, FACE_CHEW, FACE_BLINK };

// The meters that have counted their mistake for running empty (until they fill up again).
#define NOTED_FOOD   0x01
#define NOTED_FUN    0x02
#define NOTED_ENERGY 0x04

// Everything that is saved.
typedef struct {
    uint8_t  version;
    uint8_t  stage;
    uint8_t  form;
    uint8_t  level;
    uint8_t  xp;          // toward the next level
    uint8_t  food, fun, energy;
    uint8_t  poops;
    uint8_t  asleep;
    uint8_t  sick;
    uint8_t  hatch;       // the egg's warmth, up to HATCH
    uint8_t  noted;
    uint8_t  poop_in;     // seconds until the next poop; 0 when none is coming
    uint8_t  treat_run;   // treats since the last meal
    uint8_t  mistakes;    // since hatching (they decide the grown-up's form)
    uint8_t  meals;
    uint8_t  treats;
    uint8_t  catches;
    uint8_t  found;       // the forms raised to the end, one bit each: kept from egg to egg
    uint8_t  raised;      // pets raised to the end
    uint8_t  told;        // the next fact to tell: kept from egg to egg
    uint16_t starving;    // seconds with no food
    uint16_t dirty;       // seconds with poop on the floor
    uint16_t ill;         // seconds sick
    uint32_t age;         // seconds since hatching
} pet_t;

_Static_assert(sizeof(pet_t) <= PET_SAVE_SIZE, "the pet must fit in its storage");

// ---------------------------------------------------------------------------------------------
// The pictures
// ---------------------------------------------------------------------------------------------

// A body is 16 x 16, drawn at twice the size in the room. The face is drawn over it: the eyes'
// top row and their two columns, and the mouth's top row and leftmost column (it is 4 wide).
typedef struct {
    uint16_t rows[16];
    uint8_t  eye_y, eye_l, eye_r;
    uint8_t  mouth_y, mouth_x;
} body_t;

static const body_t BODIES[BODY_ADULT + NB_FORMS] = {
    {
        // baby
        {
            0b0000000000000000,
            0b0000000000000000,
            0b0000000000000000,
            0b0000000000000000,
            0b0000000000000000,
            0b0000001111000000,
            0b0000110000110000,
            0b0001000000001000,
            0b0010000000000100,
            0b0010000000000100,
            0b0100000000000010,
            0b0100000000000010,
            0b0100000000000010,
            0b0100000000000010,
            0b0010000000000100,
            0b0001111111111000,
        },
        9, 5, 10, 12, 6,
    },
    {
        // kid
        {
            0b0000000000000000,
            0b0000000000000000,
            0b0010000000000100,
            0b0011000000001100,
            0b0010100000010100,
            0b0010011111100100,
            0b0010000000000100,
            0b0100000000000010,
            0b0100000000000010,
            0b0100000000000010,
            0b0100000000000010,
            0b0100000000000010,
            0b0100000000000010,
            0b0010000000000100,
            0b0001111111111000,
            0b0001100000011000,
        },
        8, 4, 11, 11, 6,
    },
    {
        // teen
        {
            0b0000000000000000,
            0b0001000000001000,
            0b0001100000011000,
            0b0001010000101000,
            0b0001001111001000,
            0b0010000000000100,
            0b0010000000000100,
            0b0010000000000100,
            0b0010000000000100,
            0b0010000000000100,
            0b1010000000000101,
            0b0110000000000110,
            0b0010000000000100,
            0b0010000000000100,
            0b0001111111111000,
            0b0001100000011000,
        },
        7, 5, 10, 10, 6,
    },
    {
        // fluff
        {
            0b0000000000000000,
            0b0000001001000000,
            0b0000010110100000,
            0b0001100000011000,
            0b0010000000000100,
            0b0100000000000010,
            0b1000000000000001,
            0b0100000000000010,
            0b1000000000000001,
            0b0100000000000010,
            0b1000000000000001,
            0b0100000000000010,
            0b1000000000000001,
            0b0110000000000110,
            0b0001111111111000,
            0b0001100000011000,
        },
        7, 5, 10, 10, 6,
    },
    {
        // zoomie
        {
            0b0001100000011000,
            0b0010010000100100,
            0b0010010000100100,
            0b0010010000100100,
            0b0001010000101000,
            0b0001011001101000,
            0b0010000000000100,
            0b0011111111111100,
            0b0010000000000100,
            0b0010000000000100,
            0b0010000000000100,
            0b0010000000000100,
            0b0010000000000100,
            0b0001000000001000,
            0b0000111111110000,
            0b0001100000011000,
        },
        8, 5, 10, 11, 6,
    },
    {
        // chonk
        {
            0b0000000000000000,
            0b0000000000000000,
            0b0000000000000000,
            0b0011000000001100,
            0b0100111111110010,
            0b1000000000000001,
            0b1000000000000001,
            0b1000000000000001,
            0b1000000000000001,
            0b1000000000000001,
            0b1000000000000001,
            0b1000000000000001,
            0b1000000000000001,
            0b1000000000000001,
            0b0100000000000010,
            0b0011111111111100,
        },
        7, 5, 10, 10, 6,
    },
    {
        // angel
        {
            0b0000001111000000,
            0b0000010000100000,
            0b0000001111000000,
            0b0000000000000000,
            0b0000011111100000,
            0b0001100000011000,
            0b0010000000000100,
            0b1010000000000101,
            0b1110000000000111,
            0b0110000000000110,
            0b0010000000000100,
            0b0010000000000100,
            0b0010000000000100,
            0b0001000000001000,
            0b0000111111110000,
            0b0000110000110000,
        },
        7, 5, 10, 10, 6,
    },
    {
        // grump
        {
            0b0000000000000000,
            0b0001000110001000,
            0b0010101001010100,
            0b0100010000100010,
            0b0100000000000010,
            0b0101100000011010,
            0b0100010000100010,
            0b0100000000000010,
            0b0100000000000010,
            0b0100000000000010,
            0b0100000000000010,
            0b0100000000000010,
            0b0100000000000010,
            0b0010000000000100,
            0b0001111111111000,
            0b0001100000011000,
        },
        8, 5, 10, 11, 6,
    },
};

static const uint16_t EGG[16] = {
    0b0000000000000000,
    0b0000001111000000,
    0b0000010000100000,
    0b0000100000010000,
    0b0001000110001000,
    0b0001000110001000,
    0b0010011000000100,
    0b0010011000110100,
    0b0010000000110100,
    0b0010000000000100,
    0b0010000000000100,
    0b0010001100000100,
    0b0001001100001000,
    0b0001000000001000,
    0b0000100000010000,
    0b0000011111100000,
};

// The shelf's icons, 9 x 9, in the order of the actions.
static const uint16_t ICONS9[NB_DOS][9] = {
    {
        // meal
        0b000000000,
        0b000111000,
        0b011111110,
        0b111111111,
        0b100000001,
        0b100000001,
        0b010000010,
        0b001111100,
        0b000000000,
    },
    {
        // treat
        0b000011000,
        0b000110000,
        0b001111100,
        0b011111110,
        0b111111111,
        0b000000000,
        0b010101010,
        0b010101010,
        0b001111100,
    },
    {
        // play
        0b001111100,
        0b010010010,
        0b100100001,
        0b100100001,
        0b111111111,
        0b100001001,
        0b100001001,
        0b010010010,
        0b001111100,
    },
    {
        // clean
        0b000010000,
        0b000010000,
        0b000010000,
        0b000010000,
        0b001111100,
        0b011111110,
        0b010101010,
        0b101010101,
        0b101010101,
    },
    {
        // sleep
        0b001110111,
        0b011100010,
        0b111000111,
        0b111000000,
        0b111000000,
        0b111000000,
        0b111000000,
        0b011100000,
        0b001111000,
    },
    {
        // meds
        0b000000000,
        0b000111000,
        0b000101000,
        0b111101111,
        0b100000001,
        0b111101111,
        0b000101000,
        0b000111000,
        0b000000000,
    },
    {
        // info
        0b001111100,
        0b010000010,
        0b100010001,
        0b100000001,
        0b100010001,
        0b100010001,
        0b100010001,
        0b010000010,
        0b001111100,
    },
};

static const uint16_t FOOD_ICON[7] = {
    0b00010,
    0b00100,
    0b11011,
    0b11111,
    0b11111,
    0b11111,
    0b01010,
};

static const uint16_t BOLT_ICON[7] = {
    0b00011,
    0b00110,
    0b01100,
    0b11111,
    0b00110,
    0b01100,
    0b11000,
};

static const uint16_t POOP[7] = {
    0b00100000,
    0b00011000,
    0b00111100,
    0b01000010,
    0b01111110,
    0b10000001,
    0b11111111,
};

static const uint16_t BALL[5] = {
    0b01110,
    0b10111,
    0b11111,
    0b11111,
    0b01110,
};

// ---------------------------------------------------------------------------------------------
// The state
// ---------------------------------------------------------------------------------------------

#define ROOM_TOP 10
#define FLOOR    51  // the floor line: the pet stands on it
#define SHELF_Y  53
#define SCALE    2
#define PET_MIN  42  // where the pet's centre may wander
#define PET_MAX  86

#define EAT_TICKS   16
#define HAPPY_TICKS 12
#define NO_TICKS    8
#define CLEAN_TICKS 14
#define MEDS_TICKS  14
#define LEVEL_TICKS 20
#define GROW_TICKS  24
#define HATCH_TICKS 16

static pet_t   pet;
static bool    loaded;
static uint8_t mode;
static uint8_t cursor;
static uint8_t show;          // the scene playing
static uint8_t show_ticks;    // its ticks left
static uint8_t show_icon;     // its prop: the food eaten, the need refused
static uint8_t grow_from;     // the body before SHOW_GROW
static uint8_t pending_xp;    // experience given when the scene ends
static bool    ache;          // the treat being eaten is one too many
static uint8_t ticks;         // toward the next second
static uint8_t anim;          // runs freely, for the little movements
static uint8_t wobble;        // the egg was tapped: ticks of wobbling left
static int     px, target;    // the pet's centre, and where it walks to
static bool    away;          // the OS has the screen: saved once, waiting
static uint8_t browse;        // the info page's chosen slot in the collection
static bool    telling;       // a fact to tell once the scene on screen is over
static uint8_t fact;          // the fact in the box, or on the pages
static uint8_t fact_page;

// Catch.
#define THROWS     5
#define WINDOW     4   // the last ticks of a throw in which a press catches it
enum { CATCH_WAIT, CATCH_FLY, CATCH_GOT, CATCH_MISS, CATCH_END };
static uint8_t phase;
static uint8_t thrown;        // throws done
static uint8_t results;       // one bit per throw: caught
static uint8_t caught;
static int8_t  side;          // -1 from the left, +1 from the right
static uint8_t flight, flight_len;
static uint8_t countdown;

// Goodbye: the scene after the pet grew up or ran away (the new egg is already saved).
#define BYE_STAY 24  // ticks before it walks off
static uint8_t bye_body;
static bool    bye_ran;
static bool    bye_new;
static uint8_t bye_ticks;

// ---------------------------------------------------------------------------------------------
// Life
// ---------------------------------------------------------------------------------------------

static void save(void)
{
    storage_save_pet(&pet, sizeof(pet));
}

static void new_egg(void)
{
    uint8_t found  = pet.found;
    uint8_t raised = pet.raised;
    uint8_t told   = pet.told;
    memset(&pet, 0, sizeof(pet));
    pet.version = PET_VERSION;
    pet.stage   = STAGE_EGG;
    pet.level   = 1;
    pet.found   = found;
    pet.raised  = raised;
    pet.told    = told;
}

static void load(void)
{
    if (loaded) {
        return;
    }
    loaded = true;
    if (!storage_load_pet(&pet, sizeof(pet)) || pet.version != PET_VERSION) {
        memset(&pet, 0, sizeof(pet));
        new_egg();
    }
}

static void count(uint8_t *counter)
{
    if (*counter < 0xFF) {
        (*counter)++;
    }
}

static void mistake(void)
{
    count(&pet.mistakes);
}

static uint8_t stage_of(uint8_t level)
{
    return level <= 2 ? STAGE_BABY : level <= 5 ? STAGE_KID : level <= 7 ? STAGE_TEEN : STAGE_ADULT;
}

static uint8_t body_of(void)
{
    return pet.stage == STAGE_ADULT ? (uint8_t) (BODY_ADULT + pet.form) : (uint8_t) (pet.stage - 1);
}

static uint8_t xp_need(uint8_t level)
{
    return (uint8_t) (2 + 2 * level);
}

static uint8_t pick_form(void)
{
    if (pet.mistakes >= 5) {
        return FORM_GRUMP;
    }
    if (pet.treats >= 8 && pet.treats >= pet.meals) {
        return FORM_CHONK;
    }
    if (pet.catches >= 50) {
        return FORM_ZOOMIE;
    }
    if (pet.mistakes == 0) {
        return FORM_ANGEL;
    }
    return FORM_FLUFF;
}

static void play_show(uint8_t what, uint8_t length)
{
    show       = what;
    show_ticks = length;
}

static void goodbye(bool ran)
{
    bye_body  = body_of();
    bye_ran   = ran;
    bye_new   = false;
    bye_ticks = 0;
    if (!ran) {
        uint8_t bit = (uint8_t) (1 << pet.form);
        bye_new     = !(pet.found & bit);
        pet.found |= bit;
        count(&pet.raised);
    }
    new_egg();
    save();
    show       = SHOW_NONE;
    pending_xp = 0;
    ache       = false;
    telling    = false;
    mode       = MODE_BYE;
}

static void gain_xp(uint8_t n)
{
    if (pet.stage == STAGE_EGG || n == 0) {
        return;
    }
    if (show) {
        pending_xp = (uint8_t) (pending_xp + n);  // after the scene on screen
        return;
    }
    pet.xp = (uint8_t) (pet.xp + n);
    if (pet.xp < xp_need(pet.level)) {
        return;
    }
    if (pet.level == MAX_LEVEL) {
        goodbye(false);
        return;
    }
    pet.xp = (uint8_t) (pet.xp - xp_need(pet.level));
    pet.level++;
    uint8_t stage = stage_of(pet.level);
    if (stage != pet.stage) {
        grow_from = body_of();
        pet.stage = stage;
        if (stage == STAGE_ADULT) {
            pet.form = pick_form();
        }
        play_show(SHOW_GROW, GROW_TICKS);
    }
    else {
        play_show(SHOW_LEVEL, LEVEL_TICKS);
    }
    save();
}

// A meter loses a point; the first time it runs empty is a mistake.
static void drop(uint8_t *meter, uint8_t noted)
{
    if (*meter == 0) {
        return;
    }
    if (--*meter == 0 && !(pet.noted & noted)) {
        pet.noted |= noted;
        mistake();
    }
}

static void lose(uint8_t *meter, uint8_t points, uint8_t noted)
{
    while (points--) {
        drop(meter, noted);
    }
}

static void gain(uint8_t *meter, uint8_t points, uint8_t noted)
{
    *meter = (uint8_t) (*meter + points > FULL ? FULL : *meter + points);
    if (*meter >= LOW) {
        pet.noted &= (uint8_t) ~noted;
    }
}

static void fall_sick(void)
{
    if (!pet.sick) {
        pet.sick = 1;
        pet.ill  = 0;
        mistake();
    }
}

static void hatch(void)
{
    pet.stage  = STAGE_BABY;
    pet.level  = 1;
    pet.food   = 70;
    pet.fun    = 70;
    pet.energy = 90;
    px = target = GFX_W / 2;
    cursor      = DO_MEAL;
    play_show(SHOW_HATCH, HATCH_TICKS);
    save();
}

static void warm(uint8_t by)
{
    pet.hatch = (uint8_t) (pet.hatch + by);
    if (pet.hatch >= HATCH) {
        hatch();
    }
}

// Fed, happy, rested, clean and well.
static bool cared_for(void)
{
    return !pet.sick && pet.poops == 0 && pet.food >= LOW && pet.fun >= LOW &&
           (pet.asleep || pet.energy >= LOW);
}

// What it needs most, as the action that gives it.
static uint8_t need(void)
{
    if (pet.stage == STAGE_EGG) {
        return DO_NOTHING;
    }
    if (pet.sick) {
        return DO_MEDS;
    }
    if (pet.poops && pet.dirty >= DIRTY_SECS) {
        return DO_CLEAN;
    }
    if (pet.food < LOW) {
        return DO_MEAL;
    }
    if (!pet.asleep && pet.energy < LOW) {
        return DO_SLEEP;
    }
    if (pet.fun < LOW) {
        return DO_PLAY;
    }
    return DO_NOTHING;
}

static void second(void)
{
    if (pet.stage == STAGE_EGG) {
        warm(1);
        return;
    }
    pet.age++;

    uint32_t slow = pet.asleep ? 2 : 1;
    if (pet.age % (FOOD_SECS * slow) == 0) {
        drop(&pet.food, NOTED_FOOD);
    }
    if (pet.age % ((pet.sick ? FUN_SECS / 2 : FUN_SECS) * slow) == 0) {
        drop(&pet.fun, NOTED_FUN);
    }
    if (pet.asleep) {
        gain(&pet.energy, REST_GAIN, NOTED_ENERGY);
        if (pet.energy == FULL) {
            pet.asleep = 0;  // up, well rested
            play_show(SHOW_HAPPY, HAPPY_TICKS);
            pending_xp++;
        }
    }
    else if (pet.age % ENERGY_SECS == 0) {
        drop(&pet.energy, NOTED_ENERGY);
        if (pet.energy == 0) {
            pet.asleep = 1;  // falls asleep where it stands (the empty meter was the mistake)
        }
    }

    if (!pet.asleep && pet.poop_in && --pet.poop_in == 0 && pet.poops < MAX_POOPS) {
        pet.poops++;
    }
    if (pet.poops && ++pet.dirty >= SICK_SECS) {
        pet.dirty = 0;
        fall_sick();
    }
    if (pet.food == 0 && ++pet.starving >= SICK_SECS) {
        pet.starving = 0;
        fall_sick();
    }
    else if (pet.food) {
        pet.starving = 0;
    }
    if (pet.sick && ++pet.ill >= AWAY_SECS) {
        goodbye(true);
        return;
    }

    if (pet.age % CARE_SECS == 0 && cared_for()) {
        gain_xp(1);
    }
    if (pet.age % SAVE_SECS == 0) {
        save();
    }
}

static void walk(void)
{
    if (pet.stage == STAGE_EGG || pet.asleep || pet.sick || show) {
        return;
    }
    if (px == target) {
        if (random_range(0, 40) == 0) {
            target = (int) random_range(PET_MIN, PET_MAX + 1);
        }
    }
    else if (anim % 2 == 0) {
        px += px < target ? 1 : -1;
    }
}

// ---------------------------------------------------------------------------------------------
// The shelf
// ---------------------------------------------------------------------------------------------

static void refuse(uint8_t icon)
{
    show_icon = icon;
    play_show(SHOW_NO, NO_TICKS);
}

static void poop_soon(void)
{
    if (!pet.poop_in) {
        pet.poop_in = (uint8_t) random_range(40, 71);
    }
}

static void start_catch(void);

static void act(uint8_t what)
{
    if (show) {
        return;  // one thing at a time
    }
    if (what == DO_INFO) {
        mode   = MODE_INFO;
        browse = pet.stage == STAGE_ADULT ? pet.form : 0;
        return;
    }
    if (what == DO_SLEEP) {
        if (pet.asleep) {
            pet.asleep = 0;  // lights on
            telling    = true;
        }
        else if (pet.energy >= 90) {
            refuse(DO_PLAY);  // not tired: it wants to play
        }
        else {
            pet.asleep = 1;
            telling    = true;
        }
        return;
    }
    if (pet.asleep) {
        refuse(DO_SLEEP);  // shh
        return;
    }

    switch (what) {
        case DO_MEAL:
            if (pet.food >= 90) {
                refuse(DO_NOTHING);
                break;
            }
            pending_xp += pet.food < 50;
            gain(&pet.food, MEAL_FOOD, NOTED_FOOD);
            count(&pet.meals);
            pet.treat_run = 0;
            poop_soon();
            show_icon = DO_MEAL;
            play_show(SHOW_EAT, EAT_TICKS);
            telling = true;
            break;

        case DO_TREAT:
            if (pet.food >= FULL) {
                refuse(DO_NOTHING);
                break;
            }
            pending_xp += pet.fun < 50;
            gain(&pet.food, TREAT_FOOD, NOTED_FOOD);
            gain(&pet.fun, TREAT_FUN, NOTED_FUN);
            count(&pet.treats);
            if (++pet.treat_run >= TREAT_ACHE) {
                pet.treat_run = 0;
                ache          = true;
            }
            poop_soon();
            show_icon = DO_TREAT;
            play_show(SHOW_EAT, EAT_TICKS);
            telling = true;
            break;

        case DO_PLAY:
            if (pet.sick) {
                refuse(DO_MEDS);
            }
            else if (pet.energy <= PLAY_ENERGY) {
                refuse(DO_SLEEP);  // a game would wear it out
            }
            else {
                start_catch();
            }
            break;

        case DO_CLEAN:
            if (!pet.poops) {
                refuse(DO_NOTHING);
                break;
            }
            pending_xp++;
            play_show(SHOW_CLEAN, CLEAN_TICKS);
            telling = true;
            break;

        case DO_MEDS:
            if (!pet.sick) {
                refuse(DO_NOTHING);
                break;
            }
            pet.sick = 0;
            pet.ill  = 0;
            play_show(SHOW_MEDS, MEDS_TICKS);
            telling = true;
            break;

        default:
            break;
    }
}

static void show_done(void)
{
    uint8_t was = show;
    show        = SHOW_NONE;
    if (was == SHOW_EAT && ache) {
        ache = false;
        fall_sick();
    }
    if (was == SHOW_CLEAN) {
        pet.poops = 0;
        pet.dirty = 0;
    }
    uint8_t xp = pending_xp;
    pending_xp = 0;
    gain_xp(xp);
}

// ---------------------------------------------------------------------------------------------
// Catch
// ---------------------------------------------------------------------------------------------

static void next_throw(void)
{
    phase = CATCH_WAIT;
    countdown  = (uint8_t) random_range(4, 14);  // no rhythm to learn
}

static void start_catch(void)
{
    mode    = MODE_CATCH;
    thrown  = 0;
    results = 0;
    caught  = 0;
    next_throw();
    countdown += 6;
}

static void throw_ends(bool got)
{
    if (got) {
        results |= (uint8_t) (1 << thrown);
        caught++;
    }
    phase = got ? CATCH_GOT : CATCH_MISS;
    countdown  = 8;
}

static void catch_over(void)
{
    lose(&pet.energy, PLAY_ENERGY, NOTED_ENERGY);
    lose(&pet.food, PLAY_FOOD, NOTED_FOOD);
    gain(&pet.fun, (uint8_t) (CATCH_FUN * caught), NOTED_FUN);
    for (uint8_t i = 0; i < caught; i++) {
        count(&pet.catches);
    }
    mode    = MODE_ROOM;
    telling = true;
    if (caught) {
        play_show(SHOW_HAPPY, HAPPY_TICKS);
        pending_xp += caught == THROWS ? 3 : caught >= 3 ? 2 : 1;
    }
    if (pet.energy == 0) {
        pet.asleep = 1;
    }
}

static void catch_tick(void)
{
    switch (phase) {
        case CATCH_WAIT:
            if (--countdown == 0) {
                phase      = CATCH_FLY;
                flight     = 0;
                side       = random_range(0, 2) ? 1 : -1;
                flight_len = (uint8_t) (14 - pet.level / 3);
            }
            break;
        case CATCH_FLY:
            if (++flight > flight_len) {
                throw_ends(false);  // it hit the pet: too late
            }
            break;
        case CATCH_GOT:
        case CATCH_MISS:
            if (--countdown == 0) {
                if (++thrown == THROWS) {
                    phase = CATCH_END;
                    countdown  = 20;
                }
                else {
                    next_throw();
                }
            }
            break;
        default:  // CATCH_END
            if (--countdown == 0) {
                catch_over();
            }
            break;
    }
}

static void catch_event(event_t event, uint8_t buttons)
{
    if (event == EV_BACK) {
        mode = MODE_ROOM;  // no game, no cost
        return;
    }
    if (event != EV_DOWN || phase != CATCH_FLY || buttons == BTN_BOTH) {
        return;
    }
    int8_t pressed = (buttons & BTN_RIGHT) ? 1 : -1;
    throw_ends(pressed == side && flight + WINDOW > flight_len);
}

// ---------------------------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------------------------

typedef struct {
    int8_t x, y;
} dot_t;

static const dot_t EYE_OPEN[]    = {{0, 0}, {0, 1}};
static const dot_t EYE_HAPPY[]   = {{-1, 1}, {0, 0}, {1, 1}};
static const dot_t EYE_SHUT[]    = {{-1, 1}, {0, 1}, {1, 1}};
static const dot_t EYE_NO_L[]    = {{-1, 0}, {0, 1}, {-1, 2}};
static const dot_t EYE_NO_R[]    = {{1, 0}, {0, 1}, {1, 2}};
static const dot_t EYE_DIZZY[]   = {{-1, 0}, {1, 0}, {0, 1}, {-1, 2}, {1, 2}};
static const dot_t MOUTH_SMILE[] = {{0, 0}, {1, 1}, {2, 1}, {3, 0}};
static const dot_t MOUTH_OPEN[]  = {{0, 0}, {1, 0}, {2, 0}, {3, 0}, {1, 1}, {2, 1}};
static const dot_t MOUTH_FROWN[] = {{1, 0}, {2, 0}, {0, 1}, {3, 1}};
static const dot_t MOUTH_FLAT[]  = {{1, 0}, {2, 0}};
static const dot_t MOUTH_WAVY[]  = {{0, 1}, {1, 0}, {2, 1}, {3, 0}};

#define DOTS(pattern) pattern, (int) (sizeof(pattern) / sizeof(pattern[0]))

// Dots of a pattern, in units of `scale` pixels, at (ux, uy) units from (x, y).
static void dots(int x, int y, int scale, int ux, int uy, const dot_t *pattern, int n, bool on)
{
    for (int i = 0; i < n; i++) {
        gfx_fill_rect(x + (ux + pattern[i].x) * scale, y + (uy + pattern[i].y) * scale, scale, scale, on);
    }
}

static void draw_face(const body_t *b, int x, int y, int scale, uint8_t face, int look, bool on)
{
    int el = b->eye_l + look, er = b->eye_r + look, ey = b->eye_y;
    switch (face) {
        case FACE_HAPPY:
            dots(x, y, scale, el, ey, DOTS(EYE_HAPPY), on);
            dots(x, y, scale, er, ey, DOTS(EYE_HAPPY), on);
            break;
        case FACE_SLEEP:
        case FACE_BLINK:
            dots(x, y, scale, el, ey, DOTS(EYE_SHUT), on);
            dots(x, y, scale, er, ey, DOTS(EYE_SHUT), on);
            break;
        case FACE_SICK:
            dots(x, y, scale, el, ey, DOTS(EYE_DIZZY), on);
            dots(x, y, scale, er, ey, DOTS(EYE_DIZZY), on);
            break;
        case FACE_NO:
            dots(x, y, scale, el, ey, DOTS(EYE_NO_L), on);
            dots(x, y, scale, er, ey, DOTS(EYE_NO_R), on);
            break;
        default:
            dots(x, y, scale, el, ey, DOTS(EYE_OPEN), on);
            dots(x, y, scale, er, ey, DOTS(EYE_OPEN), on);
            break;
    }

    int mx = b->mouth_x, my = b->mouth_y;
    switch (face) {
        case FACE_HAPPY:
        case FACE_EAT:
            dots(x, y, scale, mx, my, DOTS(MOUTH_OPEN), on);
            break;
        case FACE_SAD:
        case FACE_NO:
            dots(x, y, scale, mx, my, DOTS(MOUTH_FROWN), on);
            break;
        case FACE_SLEEP:
        case FACE_CHEW:
            dots(x, y, scale, mx, my, DOTS(MOUTH_FLAT), on);
            break;
        case FACE_SICK:
            dots(x, y, scale, mx, my, DOTS(MOUTH_WAVY), on);
            break;
        default:
            dots(x, y, scale, mx, my, DOTS(MOUTH_SMILE), on);
            break;
    }

    // Blushing when happy: a short line under each eye, in single pixels.
    if (face == FACE_HAPPY && scale > 1) {
        gfx_fill_rect(x + (el - 1) * scale, y + (ey + 2) * scale + 1, scale + 1, 1, on);
        gfx_fill_rect(x + (er + 1) * scale - 1, y + (ey + 2) * scale + 1, scale + 1, 1, on);
    }
}

// A body with its face, standing on `bottom` and centred on cx.
static void draw_body(uint8_t body, int cx, int bottom, int scale, uint8_t face, int look, bool on)
{
    const body_t *b = &BODIES[body];
    int           x = cx - 8 * scale;
    int           y = bottom - 16 * scale;
    gfx_sprite_scaled(x, y, b->rows, 16, 16, false, on, scale);
    draw_face(b, x, y, scale, face, look, on);
}

static void draw_icon9(uint8_t icon, int x, int y, bool on)
{
    gfx_sprite(x, y, ICONS9[icon], 9, 9, false, on);
}

// A rounded bubble with an icon (or a heart) cut out of it, and a little tail toward the pet.
static void draw_need_bubble(int x, int y, uint8_t icon, bool tail_left)
{
    gfx_bubble(x, y, 13, 11, 1, true);
    if (icon == DO_NOTHING) {
        gfx_text(x + 4, y + 2, "x", 1, false);
    }
    else {
        draw_icon9(icon, x + 2, y + 1, false);
    }
    int tx = tail_left ? x : x + 12;
    gfx_pixel(tx, y + 11, true);
    gfx_pixel(tail_left ? tx - 1 : tx + 1, y + 12, true);
}

static void meter(int x, const char *glyph, const uint16_t *icon, uint8_t value)
{
    bool urgent = value < LOW;
    if (!urgent || anim % 10 < 6) {
        if (glyph) {
            gfx_text(x, 0, glyph, 1, true);
        }
        else {
            gfx_sprite(x, 0, icon, 5, 7, false, true);
        }
    }
    for (int i = 0; i < 4; i++) {
        int pip = x + 7 + i * 4;
        if (value > i * 25) {
            gfx_fill_rect(pip, 1, 3, 5, true);
        }
        else {
            gfx_rect(pip, 1, 3, 5, true);
        }
    }
}

static void level_text(char *out, uint8_t level)
{
    out[0] = 'l';
    out[1] = 'v';
    ui_number(out + 2, level, NULL);
}

static void draw_bar(void)
{
    char line[16];
    ui_bar(NULL, NULL);
    if (pet.stage == STAGE_EGG) {
        return;
    }
    level_text(line, pet.level);
    gfx_text(1, 0, line, 1, true);
    meter(52, NULL, FOOD_ICON, pet.food);
    meter(78, G_HEART, NULL, pet.fun);
    meter(104, NULL, BOLT_ICON, pet.energy);
}

static void draw_shelf(void)
{
    for (uint8_t i = 0; i < NB_DOS; i++) {
        int cx = 9 + i * 18;
        if (i == cursor) {
            gfx_bubble(cx - 7, SHELF_Y, 15, 11, 1, true);
            draw_icon9(i, cx - 4, SHELF_Y + 1, false);
        }
        else {
            draw_icon9(i, cx - 4, SHELF_Y + 1, true);
        }
    }
}

static const int8_t JUMP[HAPPY_TICKS] = {0, 2, 4, 5, 4, 2, 0, 2, 4, 5, 4, 2};

static uint8_t mood(void)
{
    if (pet.asleep) {
        return FACE_SLEEP;
    }
    switch (show) {
        case SHOW_EAT:
            return (show_ticks / 2) % 2 ? FACE_EAT : FACE_CHEW;
        case SHOW_HAPPY:
        case SHOW_LEVEL:
        case SHOW_GROW:
        case SHOW_HATCH:
        case SHOW_MEDS:
            return FACE_HAPPY;
        case SHOW_NO:
            return FACE_NO;
        default:
            break;
    }
    if (pet.sick) {
        return FACE_SICK;
    }
    if (anim % 32 < 2) {
        return FACE_BLINK;
    }
    if (need() != DO_NOTHING) {
        return FACE_SAD;
    }
    if (pet.food >= 75 && pet.fun >= 75 && pet.energy >= 75 && !pet.poops) {
        return FACE_HAPPY;
    }
    return FACE_OK;
}

// A four-pointed sparkle.
static void sparkle(int x, int y, bool on)
{
    gfx_fill_rect(x - 2, y, 5, 1, on);
    gfx_fill_rect(x, y - 2, 1, 5, on);
}

static void draw_egg(bool ink)
{
    int x = GFX_W / 2 - 16 + (wobble ? (wobble % 2 ? 1 : -1) : 0);
    int y = FLOOR - 32;
    gfx_sprite_scaled(x, y, EGG, 16, 16, false, ink, SCALE);

    // Cracks as it warms up.
    static const dot_t CRACK[] = {{3, 9}, {4, 8}, {5, 9}, {6, 8}, {7, 9}, {8, 8}, {9, 9}, {10, 8}, {11, 9}, {12, 8}};
    int cracks = pet.hatch * 3 / HATCH;  // 0, 1, 2
    if (cracks) {
        dots(x, y, SCALE, 0, 0, CRACK, cracks == 1 ? 5 : 10, ink);
    }

    // Its warmth: five hearts under it.
    for (int i = 0; i < 5; i++) {
        int hx = GFX_W / 2 - 28 + i * 12;
        if (pet.hatch * 5 / HATCH > i) {
            gfx_text(hx, SHELF_Y + 2, G_HEART, 1, true);
        }
        else {
            gfx_pixel(hx + 2, SHELF_Y + 5, true);
        }
    }
}

static void draw_poops(bool ink, int swept_to)
{
    static const int8_t SLOTS[MAX_POOPS] = {4, 112, 15};
    for (int i = 0; i < pet.poops; i++) {
        if (SLOTS[i] < swept_to) {
            continue;
        }
        gfx_sprite(SLOTS[i], FLOOR - 7, POOP, 8, 7, false, ink);
        int wave = (anim / 3 + i) % 2;  // the smell
        gfx_pixel(SLOTS[i] + 2 + wave, FLOOR - 10, ink);
        gfx_pixel(SLOTS[i] + 3 - wave, FLOOR - 11, ink);
        gfx_pixel(SLOTS[i] + 5 + wave, FLOOR - 10, ink);
        gfx_pixel(SLOTS[i] + 6 - wave, FLOOR - 11, ink);
    }
}

static void draw_room(void)
{
    bool ink = !pet.asleep;  // lights off: light ink on a dark room

    ui_page();
    draw_bar();
    if (!ink) {
        gfx_fill_rect(0, ROOM_TOP, GFX_W, FLOOR - ROOM_TOP + 1, true);
        static const uint8_t STARS[][2] = {{8, 14}, {30, 24}, {104, 13}, {120, 30}, {20, 36}};
        for (uint8_t i = 0; i < 5; i++) {
            if ((anim / 8 + i) % 4) {
                gfx_pixel(STARS[i][0], STARS[i][1], false);
            }
        }
    }
    gfx_dotted_hline(0, FLOOR, GFX_W, ink);

    if (pet.stage == STAGE_EGG) {
        draw_egg(ink);
        return;
    }

    int swept = show == SHOW_CLEAN ? (CLEAN_TICKS - show_ticks) * (GFX_W + 8) / CLEAN_TICKS : -1;
    draw_poops(ink, swept);
    if (show == SHOW_CLEAN && (swept < px - 20 || swept > px + 20)) {
        sparkle(swept, FLOOR - 4, ink);
        sparkle(swept - 5, FLOOR - 9, ink);
    }

    // The pet.
    int x = px, lift = 0;
    if (show == SHOW_NO && !pet.asleep) {
        x += show_ticks % 2 ? 1 : -1;
    }
    else if (show == SHOW_HAPPY || show == SHOW_LEVEL) {
        lift = JUMP[show_ticks % HAPPY_TICKS];
    }
    else if (px != target && !pet.asleep) {
        lift = (anim / 2) % 2;  // a hop in its step
    }
    uint8_t body = body_of();
    if (show == SHOW_GROW && show_ticks > GROW_TICKS / 3 && (show_ticks / 2) % 2) {
        body = grow_from;
    }
    draw_body(body, x, FLOOR - lift, SCALE, mood(), 0, ink);

    // What goes on around it.
    int top = FLOOR - 32;
    switch (show) {
        case SHOW_EAT: {
            int fx     = x + 17;
            int fy     = FLOOR - 12;
            int eaten  = (EAT_TICKS - show_ticks) * 10 / EAT_TICKS;  // bites, from the left
            draw_icon9(show_icon, fx, fy, ink);
            gfx_fill_rect(fx, fy, eaten, 9, !ink);
            break;
        }
        case SHOW_HAPPY:
            gfx_text(x - 2, top - 7 - lift + (show_ticks % 4 < 2), G_HEART, 1, ink);
            break;
        case SHOW_NO:
            if (show_icon != DO_NOTHING || !pet.asleep) {
                draw_need_bubble(x + 14, ROOM_TOP, show_icon, true);
            }
            break;
        case SHOW_MEDS:
            draw_icon9(DO_MEDS, x + 16, ROOM_TOP + (MEDS_TICKS - show_ticks), ink);
            if (show_ticks < MEDS_TICKS / 2) {
                sparkle(x - 18, top + 4 + show_ticks % 3, ink);
                sparkle(x + 20, top + 10 - show_ticks % 3, ink);
            }
            break;
        case SHOW_LEVEL:
            ui_bubble_text(x, ROOM_TOP - 1, G_STAR "lv up!" G_STAR, 1);
            break;
        case SHOW_CLEAN:
            break;
        case SHOW_GROW:
        case SHOW_HATCH:
            for (int i = 0; i < 4; i++) {
                int a = (show_ticks + i * 3) % 6;
                sparkle(x + (i % 2 ? 22 : -22) + (i / 2 ? a : -a), top + 4 + i * 6, ink);
            }
            break;
        default: {
            uint8_t wants = need();
            if (wants != DO_NOTHING && (!pet.sick || anim % 10 < 7)) {
                draw_need_bubble(x + 14, ROOM_TOP, wants, true);
            }
            break;
        }
    }

    // Asleep: z's float up from its head.
    if (pet.asleep) {
        for (int i = 0; i < 3; i++) {
            int rise = (anim / 2 + i * 6) % 18;
            int zy   = top + 4 - rise;
            if (zy >= ROOM_TOP + 1) {
                gfx_text(x - 22 - rise / 3, zy, "z", 1, false);
            }
        }
    }

    draw_shelf();
}

static void draw_catch(void)
{
    ui_page();
    ui_bar(NULL, NULL);
    for (uint8_t i = 0; i < THROWS; i++) {
        int  x    = 2 + i * 8;
        bool done = i < thrown || (i == thrown && (phase == CATCH_GOT || phase == CATCH_MISS));
        if (!done) {
            gfx_pixel(x + 2, 3, true);
        }
        else if (results & (1 << i)) {
            gfx_sprite(x, 1, BALL, 5, 5, false, true);
        }
        else {
            gfx_text(x, 0, "x", 1, true);
        }
    }
    gfx_dotted_hline(0, FLOOR, GFX_W, true);

    int     cx   = GFX_W / 2;
    int     hand = cx + side * 15;
    uint8_t face = FACE_OK;
    int     look = 0;
    switch (phase) {
        case CATCH_FLY: {
            int from = side > 0 ? GFX_W + 3 : -3;
            int bx   = from + (hand - from) * flight / flight_len;
            int by   = 36 - 4 * 22 * flight * (flight_len - flight) / (flight_len * flight_len);
            gfx_sprite(bx - 2, by - 2, BALL, 5, 5, false, true);
            look = side;
            break;
        }
        case CATCH_GOT:
            gfx_sprite(hand - 2, 34, BALL, 5, 5, false, true);
            gfx_text(cx - 2, FLOOR - 40, G_HEART, 1, true);
            face = FACE_HAPPY;
            break;
        case CATCH_MISS:
            gfx_sprite(hand + side * 6 - 2, FLOOR - 5, BALL, 5, 5, false, true);
            face = FACE_SAD;
            break;
        case CATCH_END:
            face = caught >= 3 ? FACE_HAPPY : caught ? FACE_OK : FACE_SAD;
            break;
        default:
            break;
    }
    int lift = phase == CATCH_GOT ? JUMP[countdown % HAPPY_TICKS] : 0;
    draw_body(body_of(), cx, FLOOR - lift, SCALE, face, look, true);

    if (phase == CATCH_END) {
        char line[16];
        ui_number(line, caught, "/5");
        ui_bubble_text(cx, ROOM_TOP + 1, line, 2);
    }
}

// The collection: one slot per form, the grown-ups raised to the end shown smiling. The chosen
// slot is framed, and how to raise that one is written under the row.
static void draw_collection(int y, uint8_t chosen)
{
    for (uint8_t i = 0; i < NB_FORMS; i++) {
        int cx = 13 + i * 25;
        if (pet.found & (1 << i)) {
            draw_body((uint8_t) (BODY_ADULT + i), cx, y + 17, 1, FACE_HAPPY, 0, true);
        }
        else {
            gfx_text_center(cx, y + 3, "?", 2, true);
        }
        if (i == chosen) {
            gfx_round_rect(cx - 11, y, 23, 20, true);
        }
    }
    gfx_text_center(GFX_W / 2, y + 23, HINTS[chosen], 1, true);
}

static int count_found(void)
{
    int n = 0;
    for (uint8_t i = 0; i < NB_FORMS; i++) {
        n += (pet.found >> i) & 1;
    }
    return n;
}

static void draw_info(void)
{
    char line[16];

    ui_page();
    if (pet.stage == STAGE_EGG) {
        ui_bar("egg", NULL);
    }
    else {
        level_text(line, pet.level);
        ui_bar(line, NULL);
        gfx_rect(30, 1, 97, 6, true);
        gfx_fill_rect(31, 2, 95 * pet.xp / xp_need(pet.level), 4, true);
    }

    draw_collection(11, browse);
    gfx_dotted_hline(0, 43, GFX_W, true);

    // How it is being raised: treats, catches and mistakes, which decide its form.
    draw_icon9(DO_TREAT, 1, 45, true);
    ui_number(line, pet.treats, NULL);
    gfx_text(12, 46, line, 1, true);
    draw_icon9(DO_PLAY, 36, 45, true);
    ui_number(line, pet.catches, NULL);
    gfx_text(47, 46, line, 1, true);
    gfx_text(72, 46, "oops", 1, true);
    ui_number(line, pet.mistakes, NULL);
    gfx_text(98, 46, line, 1, true);

    // The days of this one, and the stars of all.
    ui_rule(54);
    if (pet.stage != STAGE_EGG) {
        line[0] = 'd', line[1] = 'a', line[2] = 'y', line[3] = ' ';
        ui_number(line + 4, pet.age / DAY_SECS + 1, NULL);
        gfx_text(1, 57, line, 1, true);
    }
    ui_number(line, (uint32_t) count_found(), "/5");
    int w = gfx_text_width(line, 1);
    gfx_text(GFX_W - 1 - w - FONT_ADV * 2, 57, G_STAR, 1, true);
    gfx_text_right(GFX_W - 1, 57, line, 1, true);
}

static bool bye_walked_off(void)
{
    return bye_ticks >= BYE_STAY + 40;
}

static void draw_bye(void)
{
    ui_page();
    if (bye_walked_off()) {
        if (bye_ran) {
            // Gone: an empty room and a note on the floor.
            ui_bar(NULL, NULL);
            gfx_dotted_hline(0, FLOOR, GFX_W, true);
            gfx_rect(56, FLOOR - 9, 14, 9, true);
            gfx_fill_rect(58, FLOOR - 7, 8, 1, true);
            gfx_fill_rect(58, FLOOR - 4, 6, 1, true);
            ui_bubble_text(GFX_W / 2, 18, "bye...", 2);
        }
        else {
            ui_bar(G_STAR " grown up! " G_STAR, NULL);
            draw_collection(11, (uint8_t) (bye_body - BODY_ADULT));
            ui_bubble_text(GFX_W / 2, 47, bye_new ? G_STAR " new! " G_STAR : G_HEART " again " G_HEART, 1);
        }
        return;
    }

    ui_bar(NULL, NULL);
    gfx_dotted_hline(0, FLOOR, GFX_W, true);
    int x = GFX_W / 2;
    int lift = 0;
    if (bye_ticks >= BYE_STAY) {
        int step = (bye_ticks - BYE_STAY) * 3;
        x += bye_ran ? -step : step;
        lift = (bye_ticks / 2) % 2;
    }
    else if (!bye_ran) {
        lift = JUMP[bye_ticks % HAPPY_TICKS];
        gfx_text(x - 2, FLOOR - 40 - lift, G_HEART, 1, true);
    }
    draw_body(bye_body, x, FLOOR - lift, SCALE, bye_ran ? FACE_SAD : FACE_HAPPY, 0, true);
    if (bye_ticks < BYE_STAY && !bye_ran) {
        ui_bubble_text(x + 30, ROOM_TOP + 2, "bye!", 1);
    }
}

// ---------------------------------------------------------------------------------------------
// Facts
// ---------------------------------------------------------------------------------------------

#define FACT_LINE   20  // characters (tools/make_facts.py wraps the texts to it)
#define FACT_PAGE   6   // lines on a page, under the bar
#define FACT_LINE_H 9
#define BOX_H       48  // the box over the room; the pet sits under it
#define BOX_LINES   5
#define SCROLL_X    126 // the pages' scroll bar, 2 pixels wide

// Once the room is quiet (the scene of the action, and any level up after it, are over), the pet
// tells the next fact.
static void tell(void)
{
    if (!telling || mode != MODE_ROOM || show) {
        return;
    }
    telling  = false;
    fact     = (uint8_t) (pet.told % NB_FACTS);
    pet.told = (uint8_t) ((fact + 1) % NB_FACTS);
    mode     = MODE_FACT;
    input_swallow();  // a press already down is not for the box
}

static uint8_t fact_pages(void)
{
    return (uint8_t) ((FACTS[fact].lines + FACT_PAGE - 1) / FACT_PAGE);
}

// Copies line n of the fact's text (lines end with '\n', the text with '\0') into out.
static void fact_line(uint8_t n, char out[FACT_LINE + 1])
{
    const char *c = &FACT_TEXT[FACTS[fact].text];
    while (n && *c) {
        if (*c++ == '\n') {
            n--;
        }
    }
    int i = 0;
    while (*c && *c != '\n' && i < FACT_LINE) {
        out[i++] = *c++;
    }
    out[i] = '\0';
}

// The pet, small in the corner, says the fact in a box over the room; "more" opens it.
static void draw_fact(void)
{
    const fact_t *f = &FACTS[fact];
    char          line[FACT_LINE + 1];

    ui_page();
    gfx_round_rect(0, 0, GFX_W, BOX_H, true);
    int y = 2 + (BOX_LINES - f->fact_lines) * FACT_LINE_H / 2;
    for (uint8_t i = 0; i < f->fact_lines; i++) {
        fact_line(i, line);
        gfx_text(4, y + i * FACT_LINE_H, line, 1, true);
    }

    // The tail, down to the pet.
    gfx_fill_rect(20, BOX_H - 1, 4, 1, false);
    gfx_pixel(20, BOX_H, true);
    gfx_pixel(19, BOX_H + 1, true);
    gfx_pixel(24, BOX_H - 1, true);
    gfx_pixel(23, BOX_H, true);
    gfx_pixel(22, BOX_H, true);
    gfx_pixel(21, BOX_H + 1, true);
    gfx_pixel(20, BOX_H + 1, true);
    draw_body(body_of(), 11, GFX_H, 1, mood(), 0, true);

    ui_number(line, fact + 1u, "/");
    int n = 0;
    while (line[n]) {
        n++;
    }
    ui_number(line + n, NB_FACTS, NULL);
    gfx_text_center(60, 54, line, 1, true);
    ui_bubble_text(109, 52, "more", 1);
}

// The fact and what explains it: the concept in the bar, the pages under it.
static void draw_read(void)
{
    const fact_t *f = &FACTS[fact];
    char          line[FACT_LINE + 1];

    ui_page();
    ui_bar(&FACT_TEXT[f->title], NULL);
    uint8_t first = (uint8_t) (fact_page * FACT_PAGE);
    for (uint8_t i = 0; i < FACT_PAGE && first + i < f->lines; i++) {
        fact_line((uint8_t) (first + i), line);
        gfx_text(1, UI_TOP + 1 + i * FACT_LINE_H, line, 1, true);
    }

    // Where the page is among the fact's pages, along the right edge.
    int n = fact_pages();
    if (n > 1) {
        int track = GFX_H - UI_TOP;
        int thumb = track / n > 4 ? track / n : 4;
        for (int i = UI_TOP; i < GFX_H; i += 2) {
            gfx_pixel(SCROLL_X + 1, i, true);
        }
        gfx_fill_rect(SCROLL_X, UI_TOP + (track - thumb) * fact_page / (n - 1), 2, thumb, true);
    }
}

static void fact_event(event_t event)
{
    if (mode == MODE_FACT) {
        if (event == EV_SELECT) {
            mode      = MODE_READ;
            fact_page = 0;
        }
        else if (event != EV_DOWN) {
            mode = MODE_ROOM;  // LEFT, RIGHT or HOLD: back to the room
        }
        return;
    }
    switch (event) {
        case EV_LEFT:
            if (fact_page) {
                fact_page--;
            }
            else {
                mode = MODE_FACT;  // from the first page, back to the box
            }
            break;
        case EV_RIGHT:
            if (fact_page + 1 < fact_pages()) {
                fact_page++;
            }
            else {
                mode = MODE_ROOM;  // past the last page: read
            }
            break;
        case EV_SELECT:
        case EV_BACK:
            mode = MODE_ROOM;
            break;
        default:
            break;
    }
}

// ---------------------------------------------------------------------------------------------
// The app
// ---------------------------------------------------------------------------------------------

void pet_enter(void)
{
    load();
    mode       = MODE_ROOM;
    show       = SHOW_NONE;
    pending_xp = 0;
    ache       = false;
    telling    = false;
    ticks      = 0;
    px = target = GFX_W / 2;
}

static void leave(void)
{
    if (show) {
        show_done();  // what was given is kept
    }
    save();
    shell_back();
}

void pet_event(event_t event, uint8_t buttons)
{
    switch (mode) {
        case MODE_CATCH:
            catch_event(event, buttons);
            ui_dirty();
            return;
        case MODE_INFO:
            if (event == EV_LEFT) {
                browse = (uint8_t) ((browse + NB_FORMS - 1) % NB_FORMS);
            }
            else if (event == EV_RIGHT) {
                browse = (uint8_t) ((browse + 1) % NB_FORMS);
            }
            else if (event != EV_DOWN) {
                mode = MODE_ROOM;
            }
            ui_dirty();
            return;
        case MODE_FACT:
        case MODE_READ:
            fact_event(event);
            ui_dirty();
            return;
        case MODE_BYE:
            if (event == EV_BACK) {
                shell_back();
            }
            else if (event != EV_DOWN && bye_walked_off()) {
                mode = MODE_ROOM;
                ui_dirty();
            }
            return;
        default:
            break;
    }

    if (show == SHOW_HATCH && event != EV_BACK) {
        return;  // the last taps on the egg must not touch the shelf
    }
    switch (event) {
        case EV_BACK:
            leave();
            break;
        case EV_LEFT:
        case EV_RIGHT:
            if (pet.stage == STAGE_EGG) {
                wobble = 6;
                warm(1);
            }
            else if (event == EV_LEFT) {
                cursor = (uint8_t) ((cursor + NB_DOS - 1) % NB_DOS);
            }
            else {
                cursor = (uint8_t) ((cursor + 1) % NB_DOS);
            }
            break;
        case EV_SELECT:
            if (pet.stage == STAGE_EGG) {
                wobble = 6;
                warm(1);
            }
            else {
                act(cursor);
            }
            break;
        default:
            break;
    }
    ui_dirty();
}

void pet_tick(void)
{
    away = false;
    anim++;
    switch (mode) {
        case MODE_CATCH:
            catch_tick();
            break;
        case MODE_BYE:
            if (!bye_walked_off()) {
                bye_ticks++;
            }
            break;
        case MODE_FACT:
        case MODE_READ:
            break;  // the pet waits while it tells you a fact
        default:  // the room and the info page: life goes on
            if (wobble) {
                wobble--;
            }
            if (show && --show_ticks == 0) {
                show_done();
            }
            if (++ticks >= SECOND) {
                ticks = 0;
                second();
            }
            walk();
            tell();
            break;
    }
    ui_dirty();
}

// The OS took the screen (the device locked): save once, and start a ball in flight again.
void pet_suspend(void)
{
    if (away) {
        return;
    }
    away = true;
    if (mode == MODE_CATCH && phase == CATCH_FLY) {
        phase = CATCH_WAIT;
        countdown  = 15;
    }
    save();
}

void pet_draw(void)
{
    switch (mode) {
        case MODE_INFO:
            draw_info();
            break;
        case MODE_CATCH:
            draw_catch();
            break;
        case MODE_BYE:
            draw_bye();
            break;
        case MODE_FACT:
            draw_fact();
            break;
        case MODE_READ:
            draw_read();
            break;
        default:
            draw_room();
            break;
    }
}
