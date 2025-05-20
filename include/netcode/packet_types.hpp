#ifndef PACKET_TYPES_HPP
#define PACKET_TYPES_HPP

#pragma once
#include <cstdint>

namespace netcode {

    enum class MessageType : uint8_t {
        UNDEFINED = 0,
        ECHO_REQUEST,
        ECHO_RESPONSE,
        SERVER_ANNOUNCEMENT,
        PLAYER_STATE_UPDATE,
        // Future types for Phase 2 and beyond:
        // HANDSHAKE_HELLO,
        // HANDSHAKE_WELCOME,
        // PLAYER_INPUT,
        // GAME_EVENT
    };

    struct PacketHeader {
        MessageType type = MessageType::UNDEFINED;
        uint32_t sequenceNumber = 0;
    };

    struct ServerAnnouncementData {
        std::string message_text;
    };
}
#endif
