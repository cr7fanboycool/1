namespace ragebot {
    void apply_anti_aim(CUserCmd* cmd, bool& send_packet) {
        if (!menu::hvh::anti_aim) return;

        static float spin_angle = 0.f;
        static bool flip_jitter = false;
        
        // Pitch control
        switch (menu::hvh::pitch_mode) {
            case 1: cmd->viewangles.x = 89.f; break;  // Down
            case 2: cmd->viewangles.x = -89.f; break; // Up
        }

        // Yaw control
        switch (menu::hvh::yaw_mode) {
            case 0: // Static
                cmd->viewangles.y += menu::hvh::yaw_offset;
                break;
                
            case 1: // Jitter
                flip_jitter = !flip_jitter;
                cmd->viewangles.y += flip_jitter ? menu::hvh::yaw_offset : -menu::hvh::yaw_offset;
                break;
                
            case 2: // Spin
                spin_angle += 15.f;
                if (spin_angle > 360.f) spin_angle -= 360.f;
                cmd->viewangles.y = spin_angle;
                break;
        }

        // Fake duck
        if (menu::hvh::fake_duck) {
            static bool duck_flip = false;
            duck_flip = !duck_flip;
            cmd->buttons |= duck_flip ? IN_DUCK : 0;
        }
    }
}