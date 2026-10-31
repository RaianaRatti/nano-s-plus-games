// Random numbers from the secure element's true random number generator. The dice in BadgeOS
// used the ESP32's hardware source the same way.

#include "os.h"
#include "lcx_rng.h"

#include "core.h"

uint32_t random_range(uint32_t low, uint32_t high)
{
    return cx_rng_u32_range(low, high);
}
