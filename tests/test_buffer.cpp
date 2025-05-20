#include "gtest/gtest.h"
#include "netcode/serialization.hpp"

// Test fixture for Buffer tests
class BufferTest : public ::testing::Test {
protected:
    netcode::Buffer buffer;
};

// Test case for writing and reading uint32_t
TEST_F(BufferTest, ReadWriteUint32) {
    uint32_t value_to_write = 123456789;
    buffer.write_uint32_t(value_to_write);
    ASSERT_EQ(buffer.get_size(), sizeof(uint32_t));

    uint32_t read_value = buffer.read_uint32_t();
    ASSERT_EQ(read_value, value_to_write);
    ASSERT_EQ(buffer.read_offset, sizeof(uint32_t));
}

// Test case for writing and reading a string
TEST_F(BufferTest, ReadWriteString) {
    std::string str_to_write = "Hello, Netcode!";
    buffer.write_string(str_to_write);

    // Size should be sizeof(uint32_t) for length + string length
    ASSERT_EQ(buffer.get_size(), sizeof(uint32_t) + str_to_write.length());

    std::string read_str = buffer.read_string();
    ASSERT_EQ(read_str, str_to_write);
    ASSERT_EQ(buffer.read_offset, sizeof(uint32_t) + str_to_write.length());
}

// Test case for reading when buffer is too small (underflow)
TEST_F(BufferTest, ReadUint32Underflow) {
    // Buffer is empty
    ASSERT_THROW(buffer.read_uint32_t(), std::runtime_error);
}

TEST_F(BufferTest, ReadStringUnderflowLength) {
    buffer.write_uint8(1); // Not enough for a uint32_t length
    ASSERT_THROW(buffer.read_string(), std::runtime_error);
}

TEST_F(BufferTest, ReadStringUnderflowData) {
    std::string test_str = "short";
    // Write a length that's too long for the actual data
    buffer.write_uint32_t(static_cast<uint32_t>(test_str.length() + 5));
    buffer.data.insert(buffer.data.end(), test_str.begin(), test_str.end()); // Manually insert string data

    ASSERT_THROW(buffer.read_string(), std::runtime_error);
}


// Test case for PacketHeader
TEST_F(BufferTest, ReadWriteHeader) {
    netcode::PacketHeader header_to_write;
    header_to_write.type = netcode::MessageType::ECHO_REQUEST;
    header_to_write.sequenceNumber = 101;

    buffer.write_header(header_to_write);
    ASSERT_EQ(buffer.get_size(), sizeof(uint8_t) + sizeof(uint32_t)); // type + sequenceNumber

    netcode::PacketHeader read_header = buffer.read_header();
    ASSERT_EQ(static_cast<uint8_t>(read_header.type), static_cast<uint8_t>(netcode::MessageType::ECHO_REQUEST));
    ASSERT_EQ(read_header.sequenceNumber, header_to_write.sequenceNumber);
    ASSERT_EQ(buffer.read_offset, sizeof(uint8_t) + sizeof(uint32_t));
}