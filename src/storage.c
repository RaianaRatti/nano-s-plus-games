// High scores, in the app's own flash (the SDK's app storage). They survive turning the device
// off; they are lost when the app is uninstalled. A failure never stops a game.
//
// One uint32 per game, in the order of best_t. The storage only lets the app read what it has
// written, so a new best rewrites every slot: after the first write all of them can be read.

#include "os.h"
#include "app_storage.h"

#include "arcade.h"

uint32_t storage_best(best_t slot)
{
    uint32_t best   = 0;
    uint32_t offset = (uint32_t) slot * sizeof(best);
    if (app_storage_read(&best, sizeof(best), offset) != (int32_t) sizeof(best)) {
        return 0;
    }
    return best;
}

void storage_set_best(best_t slot, uint32_t score)
{
    uint32_t all[NB_BEST];
    for (int i = 0; i < NB_BEST; i++) {
        all[i] = storage_best((best_t) i);
    }
    all[slot] = score;
    app_storage_write(all, sizeof(all), 0);
    app_storage_increment_data_version();
}
