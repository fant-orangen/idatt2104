#ifndef PACKET_TYPES_HPP
#define PACKET_TYPES_HPP

#pragma once
#include <cstdint>
#include <string>

namespace netcode {

    // Forward declare Buffer class
    class Buffer;

    enum class MessageType : uint32_t {
        NONE = 0,
        ECHO_REQUEST = 1,
        ECHO_RESPONSE = 2,
        PLAYER_MOVEMENT_UPDATE = 3,  // Client → Server: Movement intent
        PLAYER_POSITION_UPDATE = 4,  // Server → Clients: Authoritative position
        SERVER_ANNOUNCEMENT,
        // Future types for Phase 2 and beyond:
        // HANDSHAKE_HELLO,
        // HANDSHAKE_CHALLENGE,
        // HANDSHAKE_RESPONSE,
        // CONNECT_REQUEST,
        // CONNECT_ACCEPTED,
        // CONNECT_REJECTED,
        // DISCONNECT,
    };

    struct PacketHeader {
        MessageType type;
        uint32_t sequenceNumber;
    };

    struct ServerAnnouncementData {
        std::string message_text;
    };

    // Forward declare serialization functions - implemented elsewhere
    void serialize(Buffer& buf, const PacketHeader& header);
    PacketHeader deserialize_header(Buffer& buf);
}
#endif
