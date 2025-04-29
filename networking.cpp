namespace networking {
    int choked_packets = 0;
    
    bool should_choke_packet() {
        if (!menu::hvh::fakelag) return false;
        
        // Don't lag while standing still
        if (local_player.velocity().length_2d() < 5.f) 
            return false;

        // Dynamic variance
        int variance = (menu::hvh::fakelag_mode == 1) ? 
            (rand() % 5 - 2) : 0; // -2 to +2
            
        return choked_packets < (menu::hvh::fakelag_amount + variance);
    }

    void handle_outgoing_packet(bool& send_packet) {
        if (should_choke_packet()) {
            send_packet = false;
            choked_packets++;
        } else {
            send_packet = true;
            choked_packets = 0;
        }
    }
}