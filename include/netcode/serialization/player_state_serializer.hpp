#pragma once
#include "netcode/serialization.hpp"
#include "netcode/packets/player_state_packet.hpp"

namespace netcode::serialization {

    inline void serialize(Buffer& buffer, const packets::PlayerStatePacket& packet) {
        buffer.write(packet.player_id);
        buffer.write(packet.x);
        buffer.write(packet.y);
        buffer.write(packet.z);
        buffer.write(packet.velocity_y);
        buffer.write(packet.is_jumping);
    }

    inline bool deserialize(Buffer& buffer, packets::PlayerStatePacket& packet) {
        if (!buffer.read(packet.player_id)) return false;
        if (!buffer.read(packet.x)) return false;
        if (!buffer.read(packet.y)) return false;
        if (!buffer.read(packet.z)) return false;
        if (!buffer.read(packet.velocity_y)) return false;
        if (!buffer.read(packet.is_jumping)) return false;
        return true;
    }

}
