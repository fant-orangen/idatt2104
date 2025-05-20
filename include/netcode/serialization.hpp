#ifndef SERIALIZATION_HPP
#define SERIALIZATION_HPP


#pragma once

#include "packet_types.hpp"
#include <vector>
#include <string>
#include <stdexcept> // For std::runtime_error
#include <cstring>   // For std::memcpy
#include <algorithm> // For std::copy

namespace netcode {
class Buffer {
    public:
        std::vector<char> data;
        size_t read_offset = 0;

        Buffer() = default;

        Buffer(const char* existing_data, size_t size) : data(existing_data, existing_data + size) {}
        Buffer(const std::vector<char>& existing_data) : data(existing_data) {}

        void clear() {
            data.clear();
            read_offset = 0;
        }

        const char* get_data() const {
            return data.data();
        }

        char* get_mutable_data() {
            return data.data();
        }

        size_t get_size() const {
            return data.size();
        }

        void resize(size_t new_size) {
            data.resize(new_size);
        }

        void write_uint8(uint8_t value) {
            data.push_back(static_cast<char>(value));
        }

        void write_uint32_t(uint32_t value) {
            const char* bytes = reinterpret_cast<const char*>(&value);
            data.insert(data.end(), bytes, bytes + sizeof(uint32_t));
        }

        void write_string(const std::string& str) {
            uint32_t len = static_cast<uint32_t>(str.length());
            write_uint32_t(len); // Write length of the string first
            data.insert(data.end(), str.begin(), str.end()); // Then write string data
        }

        void write_header(const PacketHeader& header) {
            write_uint8(static_cast<uint8_t>(header.type));
            write_uint32_t(header.sequenceNumber);
        }

        uint8_t read_uint8() {
            if (read_offset + sizeof(uint8_t) > data.size()) {
                throw std::runtime_error("Buffer underflow: not enough data in buffer to read uint8_t");
            }

            uint8_t value = static_cast<uint8_t>(data[read_offset]);
            read_offset += sizeof(uint8_t);
            return value;
        }

        uint32_t read_uint32_t() {
            if (read_offset + sizeof(uint32_t) > data.size()) {
                throw std::runtime_error("Buffer underflow: not enough data in buffer to read uint32_t");
            }
            uint32_t value;
            std::memcpy(&value, data.data() + read_offset, sizeof(uint32_t));
            read_offset += sizeof(uint32_t);
            return value;
        }

        std::string read_string() {
            uint32_t len = read_uint32_t();
            if (read_offset + len > data.size()) {
                throw std::runtime_error("Buffer underflow: not enough data for string of length " + std::to_string(len));
            }
            std::string str(data.data() + read_offset, len);
            read_offset += len;
            return str;
        }

        PacketHeader read_header() {
            PacketHeader header;
            header.type = static_cast<MessageType>(read_uint8());
            header.sequenceNumber = read_uint32_t();
            return header;
        }
};

inline void serialize(Buffer& buffer, const PacketHeader& header) {
    buffer.write_uint8(static_cast<uint8_t>(header.type));
    buffer.write_uint32(header.sequenceNumber);
}

inline bool deserialize(Buffer& buffer, PacketHeader& header) {
    uint8_t type_val;
    if (!buffer.read_uint8(type_val)) {
        return false;
    }
}

inline void serialize(Buffer& buffer, const ServerAnnouncementData& payload) {
    uint32_t message_len = static_cast<uint32_t>(payload.message_text.length());
    buffer.write_uint32(message_len);
    buffer.write_bytes(payload.message_text.data(), message_len);
}

inline deserialize(Buffer& buffer, ServerAnnouncementData& payload) {
    uint32_t message_len;
    if (!buffer.read_uint32(message_len)) {
        return false;
    }
}

const size_t MAX_ANNOUNCEMENT_MSG_LEN = 1024;
if (message_len > MAX_ANNOUNCEMENT_MSG_LEN) {
    std::cerr << "Serialization Error: Announcement message length (" << message_len
    << ") exceeds maximum allowed (" << MAX_ANNOUNCEMENT_MSG_LEN << ")." << std::endl;
    return false;
}
if (message_len == 0) {
    payload.message_text.clear();
    return true;
}

std::vector<char> temp_message_buffer(message_len);
if (!buffer.read_bytes(temp_message_buffer.data(), message_len)) {
    return false;
}

if (message_len == 0) {
    payload.message_text.assign(temp_message_buffer.data(), message_len);
    return true;
}
payload.message_text.resize(message_len);
buffer.read_bytes(&payload.message_text[0], message_len);
return true;
} catch (const std::runtime_error& e) {
    return false;

}

template<typename TPayload>
void pack_message()

#endif SERIALIZATION_HPP
