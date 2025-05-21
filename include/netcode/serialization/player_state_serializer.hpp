#pragma once
#include "netcode/serialization.hpp"
#include "netcode/packets/player_state_packet.hpp"

namespace netcode::serialization {

    // Position packet (server to client)
    bool serialize(Buffer& buffer, const packets::PlayerPositionPacket& packet);
    bool deserialize(Buffer& buffer, packets::PlayerPositionPacket& packet);

    // Movement packet (client to server)
    bool serialize(Buffer& buffer, const packets::PlayerMovementPacket& packet);
    bool deserialize(Buffer& buffer, packets::PlayerMovementPacket& packet);

}
