// File: Scene.h

#ifndef SCENE_H
#define SCENE_H

#include "led.h"

typedef uint32_t SCENE_ID;
typedef uint32_t SCENE;

#define FLAGS_END      0xFF
#define FLAGS_END_ALL  0xFE
#define FLAGS_END_LAST 0xFD
#define FLAGS_SKIP     0xFC


// Render scene given scene id.
//
//extern void Set_Scene_idx(int /*phy_mask*/, SCENE_ID /*id*/);

extern void Set_Scene_mask(uint32_t /*phy_mask*/, SCENE_ID /*scene_id*/);

extern void Render_Scene_ptr(LED* /*leds*/, size_t /*num_leds*/, SCENE_ID /*id*/);

#endif // SCENE_H

// Endfile: Scene.h
