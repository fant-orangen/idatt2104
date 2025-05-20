#include "netcode/serialization/player_state_serializer.hpp"

namespace netcode::serialization {

    bool serialize(Buffer& buffer, const packets::PlayerStatePacket& packet) {
        buffer.write(packet.player_id);
        buffer.write(packet.x);
        buffer.write(packet.y);
        buffer.write(packet.z);
        buffer.write(packet.velocity_y);
        buffer.write(packet.is_jumping);

        return true;
    }

    bool deserialize(Buffer& buffer, packets::PlayerStatePacket& packet) {

        if (buffer.get_remaining() <
            (sizeof(uint32_t) + 4 * sizeof(float) + sizeof(bool))) {
            return false;
        }

        buffer.read(packet.player_id);
        buffer.read(packet.x);
        buffer.read(packet.y);
        buffer.read(packet.z);
        buffer.read(packet.velocity_y);
        buffer.read(packet.is_jumping);

        return true;
    }

}
