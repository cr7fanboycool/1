namespace resolver {
    struct PlayerData {
        float last_move_time = 0.f;
        Vector last_eye_angles = {0,0,0};
        int resolve_mode = 0;
    };

    std::array<PlayerData, 65> player_db;

    void resolve_entity(Player* entity) {
        if (!menu::hvh::resolver || !entity->is_player()) 
            return;

        int index = entity->index();
        auto& data = player_db[index];
        
        // Basic movement analysis
        if ((global_vars->curtime - data.last_move_time) > 0.1f) {
            data.resolve_mode = 1; // Stationary
        } else {
            data.resolve_mode = 2; // Moving
        }

        // Apply resolution
        switch (data.resolve_mode) {
            case 1: entity->eye_angles().y = data.last_eye_angles.y; break;
            case 2: entity->eye_angles().y += 58.f; break;
        }

        // Update history
        data.last_move_time = global_vars->curtime;
        data.last_eye_angles = entity->eye_angles();
    }
}