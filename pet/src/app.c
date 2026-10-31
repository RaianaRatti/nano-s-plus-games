// The app around the pet: it opens straight into the pet's room, and HOLD there (after saving)
// goes back to the Ledger dashboard.
//
// The pet is kept in the app's own flash (the SDK's app storage). It survives turning the device
// off; it is lost when the app is uninstalled. A failure to save never stops the pet.

#include "os.h"
#include "main_std_app.h"
#include "app_storage.h"

#include "pet.h"

void shell_init(void)
{
    input_reset();
    pet_enter();
    ui_dirty();
}

void shell_event(event_t event, uint8_t buttons)
{
    pet_event(event, buttons);
}

void shell_tick(void)
{
    pet_tick();
}

void shell_suspend(void)
{
    pet_suspend();
}

void shell_render(void)
{
    if (!ui_take_dirty()) {
        return;
    }
    pet_draw();
    gfx_present();
}

void shell_back(void)
{
    app_exit();
}

bool storage_load_pet(void *pet, size_t size)
{
    return app_storage_read(pet, size, 0) == (int32_t) size;
}

void storage_save_pet(const void *pet, size_t size)
{
    app_storage_write(pet, size, 0);
    app_storage_increment_data_version();
}
