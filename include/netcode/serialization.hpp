#ifndef SERIALIZATION_HPP
#define SERIALIZATION_HPP

#pragma once

#include <vector>
#include <string>
#include <stdexcept> // For std::runtime_error
#include <cstring>   // For std::memcpy
#include <cstdint>   // For uint8_t, uint32_t, uint64_t
#include <bit>       // C++20: std::endian

// For htonl/ntohl etc.
#if defined(_WIN32) || defined(_WIN64)
#   include <winsock2.h> // For Windows
    // Link with Ws2_32.lib if not using MSVC's pragma comment or auto-linking
#else
#   include <arpa/inet.h> // For POSIX systems (Linux, macOS)
#endif

#include "packet_types.hpp"  // PacketHeader, ServerAnnouncementData, MessageType

namespace netcode {

class Buffer {
public:
    // Maximum string length to prevent excessive allocations from malicious packets.
    static constexpr size_t MAX_STRING_LENGTH = 4096;

    //--- Constructors ---
    Buffer() = default;
    explicit Buffer(const char* bytes, size_t len)
      : data_(bytes, bytes + len), read_offset_(0) {}
    explicit Buffer(const std::vector<char>& v)
      : data_(v), read_offset_(0) {}

    //--- Accessors ---
    const char* get_data() const noexcept { return data_.data(); }
    size_t      get_size() const noexcept { return data_.size(); }
    size_t      get_remaining() const noexcept {
        return (read_offset_ >= data_.size())
               ? 0
               : data_.size() - read_offset_;
    }

    //--- Buffer management ---
    void clear()   noexcept { data_.clear(); read_offset_ = 0; }
    void reserve(size_t n)   { data_.reserve(n); }

    //--- Write primitives (network byte order for multi-byte integers) ---
    void write_uint8(uint8_t v) {
        data_.push_back(static_cast<char>(v));
    }

    void write_uint32(uint32_t host_val) {
        uint32_t net_val = htonl(host_val);
        const char* p = reinterpret_cast<const char*>(&net_val);
        data_.insert(data_.end(), p, p + sizeof(net_val));
    }

    void write_uint64(uint64_t host_val) {
        uint64_t net_val = host_to_network64(host_val);
        const char* p = reinterpret_cast<const char*>(&net_val);
        data_.insert(data_.end(), p, p + sizeof(net_val));
    }

    void write_bytes(const char* bytes, size_t len) {
        if (bytes && len > 0) // Ensure len > 0 as well
            data_.insert(data_.end(), bytes, bytes + len);
    }

    void write_string(const std::string& s) {
        // Optional: Check if s.size() > MAX_STRING_LENGTH on write side
        // if (s.size() > MAX_STRING_LENGTH) {
        //     throw std::runtime_error("Buffer write_string: string length exceeds maximum");
        // }
        write_uint32(static_cast<uint32_t>(s.size()));
        write_bytes(s.data(), s.size());
    }

    void write_header(const PacketHeader& h) {
        write_uint8(static_cast<uint8_t>(h.type));
        write_uint32(h.sequenceNumber);
    }

    //--- Read primitives (throw on underflow, convert from network byte order) ---
    uint8_t read_uint8() {
        if (read_offset_ + sizeof(uint8_t) > data_.size())
            throw std::runtime_error("Buffer underflow reading uint8. Offset: " + std::to_string(read_offset_) + ", Size: " + std::to_string(data_.size()));
        return static_cast<uint8_t>(data_[read_offset_++]);
    }

    uint32_t read_uint32() {
        if (read_offset_ + sizeof(uint32_t) > data_.size())
            throw std::runtime_error("Buffer underflow reading uint32. Offset: " + std::to_string(read_offset_) + ", Size: " + std::to_string(data_.size()));
        uint32_t net_val;
        std::memcpy(&net_val, data_.data() + read_offset_, sizeof(net_val));
        read_offset_ += sizeof(net_val);
        return ntohl(net_val);
    }

    uint64_t read_uint64() {
        if (read_offset_ + sizeof(uint64_t) > data_.size())
            throw std::runtime_error("Buffer underflow reading uint64. Offset: " + std::to_string(read_offset_) + ", Size: " + std::to_string(data_.size()));
        uint64_t net_val;
        std::memcpy(&net_val, data_.data() + read_offset_, sizeof(net_val));
        read_offset_ += sizeof(net_val);
        return network_to_host64(net_val);
    }

    void read_bytes(char* out, size_t len) {
        if (len == 0) return; // Nothing to read if len is 0
        if (!out) // If len > 0, out must not be null
            throw std::runtime_error("Buffer read_bytes: null destination for non-zero length");
        if (read_offset_ + len > data_.size())
            throw std::runtime_error("Buffer underflow reading bytes. Required: " + std::to_string(len) +
                                     ", Available: " + std::to_string(get_remaining()) +
                                     ", Offset: " + std::to_string(read_offset_) + ", Size: " + std::to_string(data_.size()));
        std::memcpy(out, data_.data() + read_offset_, len);
        read_offset_ += len;
    }

    std::string read_string() {
        // Peek length without advancing offset initially to validate
        if (read_offset_ + sizeof(uint32_t) > data_.size())
            throw std::runtime_error("Buffer underflow reading string length. Offset: " + std::to_string(read_offset_) + ", Size: " + std::to_string(data_.size()));

        uint32_t net_len;
        std::memcpy(&net_len, data_.data() + read_offset_, sizeof(net_len));
        uint32_t len = ntohl(net_len); // Convert peeked length to host order

        if (len > MAX_STRING_LENGTH)
            throw std::runtime_error("Buffer read_string: string length " + std::to_string(len) + " exceeds maximum " + std::to_string(MAX_STRING_LENGTH));

        // Now check if the full string data (including length prefix) is available
        if (read_offset_ + sizeof(uint32_t) + len > data_.size())
            throw std::runtime_error("Buffer underflow reading string data. Required for data: " + std::to_string(len) +
                                     ", Available after length: " + std::to_string(data_.size() - (read_offset_ + sizeof(uint32_t))) );

        // Consume the length from the buffer by advancing offset
        read_offset_ += sizeof(uint32_t);

        if (len == 0) return {}; // Return empty string if length is zero

        std::string s(len, '\0'); // Initialize string of specific size
        // memcpy is fine here, or call this->read_bytes(&s[0], len);
        std::memcpy(&s[0], data_.data() + read_offset_, len);
        read_offset_ += len;
        return s;
    }

    PacketHeader read_header() {
        PacketHeader h;
        h.type           = static_cast<MessageType>(read_uint8());
        h.sequenceNumber = read_uint32();
        return h;
    }

    /*void write(float value) {
        const char* bytes = reinterpret_cast<const char*>(&value);
        data_.insert(data_.end(), bytes, bytes + sizeof(float));
    }

    bool read(float& value) {
        if (read_offset_ + sizeof(float) > data_.size()) {
            return false;
        }
        std::memcpy(&value, data_.data() + read_offset_, sizeof(float));
        read_offset_ += sizeof(float);
        return true;
    }

    void write(bool value) {
        data_.push_back(value ? 1 : 0);
    }

    bool read(bool& value) {
        if (read_offset_ > data_.size()) {
            return false;
        }

        value = (data_[read_offset_++] != 0);
        return true;
    }

    void write(uint32_t value) {
        write_uint32(value);
    }

    bool read(uint32_t& value) {
        if (read_offset_ + sizeof(uint32_t) > data_.size()) {
            return false;
        }
        value = read_uint32();
        return true;
    }*/


private:
    // Byte-swap helper for 64-bit integers (constexpr for potential compile-time use)
    static constexpr uint64_t byteswap64(uint64_t val) noexcept {
    #if defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap64(val);
    #elif defined(_MSC_VER)
        return _byteswap_uint64(val);
    #else // Fallback manual swap
        val = (val & 0x00000000FFFFFFFFULL) << 32 | (val & 0xFFFFFFFF00000000ULL) >> 32;
        val = (val & 0x0000FFFF0000FFFFULL) << 16 | (val & 0xFFFF0000FFFF0000ULL) >> 16;
        val = (val & 0x00FF00FF00FF00FFULL) <<  8 | (val & 0xFF00FF00FF00FF00ULL) >>  8;
        return val;
    #endif
    }

    // C++20 endian-based conversions for 64-bit integers
    static constexpr uint64_t host_to_network64(uint64_t host_val) noexcept {
        if constexpr (std::endian::native == std::endian::little) {
            return byteswap64(host_val);
        } else { // Already big-endian (network order) or mixed (not handled, assume big for network)
            return host_val;
        }
    }

    static constexpr uint64_t network_to_host64(uint64_t net_val) noexcept {
        // Conversion is symmetrical
        if constexpr (std::endian::native == std::endian::little) {
            return byteswap64(net_val);
        } else {
            return net_val;
        }
    }
    std::vector<char> data_;
    size_t            read_offset_ = 0;
}; // End of Buffer class

//--- Free helper functions for deserialization (swallow exceptions) ---
inline void serialize(Buffer& buf, const PacketHeader& h) {
    buf.write_header(h); // Delegates to Buffer's own method
}

inline bool try_deserialize(Buffer& buf, PacketHeader& h) {
    try {
        h = buf.read_header(); // Delegates to Buffer's own method
        return true;
    } catch (const std::runtime_error& /*e*/) { // Catch specific known exception
        // Optional: Log e.what() for debugging
        return false;
    }
}

inline void serialize(Buffer& buf, const ServerAnnouncementData& m) {
    buf.write_string(m.message_text); // Delegates to Buffer's own method
}

inline bool try_deserialize(Buffer& buf, ServerAnnouncementData& m) {
    try {
        m.message_text = buf.read_string(); // Delegates to Buffer's own method
        return true;
    } catch (const std::runtime_error& /*e*/) { // Catch specific known exception
        // Optional: Log e.what() for debugging
        return false;
    }
}

} // namespace netcode

#endif // SERIALIZATION_HPP