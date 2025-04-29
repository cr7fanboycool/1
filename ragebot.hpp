#pragma once

namespace ragebot {
    // Configurable settings
    inline bool enabled = false;
    inline bool double_tap = true;
    inline int  shift_ticks = 14;
    inline float hit_chance = 80.f;

    void run(void* cmd);
}