#pragma once
#include "netcode/serialization.hpp"
#include "netcode/packets/player_state_packet.hpp"

namespace netcode::serialization {

    bool serialize(Buffer& buffer, const packets::PlayerStatePacket& packet);

    bool deserialize(Buffer& buffer, packets::PlayerStatePacket& packet);

}
