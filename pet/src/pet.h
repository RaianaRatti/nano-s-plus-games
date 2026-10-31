// The pet: a creature to raise, as an app of its own on the core (core.h).
//
//   pet.c   its life, the room, catch, the collection, the facts it tells
//   facts.c the physics facts, made by tools/make_facts.py from pet/facts/
//   app.c   the app around it: the shell the core calls, and the pet's flash

#pragma once

#include "core.h"

void pet_enter(void);
void pet_event(event_t event, uint8_t buttons);
void pet_tick(void);
void pet_suspend(void);
void pet_draw(void);

// The pet in the app's flash: up to PET_SAVE_SIZE bytes. Loading returns false when it was never
// saved.
#define PET_SAVE_SIZE 64

bool storage_load_pet(void *pet, size_t size);
void storage_save_pet(const void *pet, size_t size);
