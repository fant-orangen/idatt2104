#ifndef PACKET_TYPES_HPP
#define PACKET_TYPES_HPP

#endif //PACKET_TYPES_HPP

#pragma once
#include <cstdint>

namespace netcode {

    enum class MessageType : uint8_t {
        UNDEFINED = 0,
        ECHO_REQUEST,
        ECHO_RESPONSE,
        // Future types for Phase 2 and beyond:
        // HANDSHAKE_HELLO,
        // HANDSHAKE_WELCOME,
        // PLAYER_INPUT,
        // PLAYER_STATE_UPDATE,
        // GAME_EVENT
    };

    struct PacketHeader {
        MessageType type = MessageType::UNDEFINED;
        uint32_t sequenceNumber = 0;
    };
}
