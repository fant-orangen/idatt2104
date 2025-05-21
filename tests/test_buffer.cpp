#include "gtest/gtest.h"
#include "netcode/serialization.hpp" //
#include "netcode/packet_types.hpp" // For MessageType, PacketHeader
#include <cstring>                  // For std::strlen, std::memcmp
#include <vector>
#include <limits>                   // For std::numeric_limits (if you add boundary tests)


// Test fixture for Buffer tests
class BufferTest : public ::testing::Test {
protected:
    netcode::Buffer buffer; //
};

// Test case for writing and reading uint32_t
TEST_F(BufferTest, ReadWriteUint32) {
    uint32_t value_to_write = 123456789;
    buffer.write_uint32(value_to_write); //
    ASSERT_EQ(buffer.get_size(), sizeof(uint32_t)); //

    uint32_t read_value = buffer.read_uint32(); //
    ASSERT_EQ(read_value, value_to_write);
    ASSERT_EQ(buffer.get_remaining(), 0); // FIXED: Check remaining bytes //
}

// Test case for writing and reading a string
TEST_F(BufferTest, ReadWriteString) {
    std::string str_to_write = "Hello, Netcode!";
    buffer.write_string(str_to_write); //
    ASSERT_EQ(buffer.get_size(), sizeof(uint32_t) + str_to_write.length()); //
    std::string read_str = buffer.read_string(); //
    ASSERT_EQ(read_str, str_to_write);
    ASSERT_EQ(buffer.get_remaining(), 0); // FIXED: Check remaining bytes //
}

// Test case for reading when buffer is too small (underflow)
TEST_F(BufferTest, ReadUint32Underflow) {
    ASSERT_THROW(buffer.read_uint32(), std::runtime_error); //
}

TEST_F(BufferTest, ReadStringUnderflowLength) {
    buffer.write_uint8(1); //
    ASSERT_THROW(buffer.read_string(), std::runtime_error); //
}

TEST_F(BufferTest, ReadStringUnderflowData) {
    std::string test_str_data = "short";
    uint32_t declared_length = static_cast<uint32_t>(test_str_data.length() + 5);
    buffer.write_uint32(declared_length); //
    buffer.write_bytes(test_str_data.data(), test_str_data.length()); //
    ASSERT_THROW(buffer.read_string(), std::runtime_error); //
}


// Test case for PacketHeader
TEST_F(BufferTest, ReadWriteHeader) {
    netcode::PacketHeader header_to_write; //
    header_to_write.type = netcode::MessageType::ECHO_REQUEST; //
    header_to_write.sequenceNumber = 101; //

    buffer.write_header(header_to_write); //
    ASSERT_EQ(buffer.get_size(), sizeof(uint8_t) + sizeof(uint32_t)); // type + sequenceNumber //

    netcode::PacketHeader read_header = buffer.read_header(); //
    ASSERT_EQ(static_cast<uint8_t>(read_header.type), static_cast<uint8_t>(netcode::MessageType::ECHO_REQUEST));
    ASSERT_EQ(read_header.sequenceNumber, header_to_write.sequenceNumber);
    ASSERT_EQ(buffer.get_remaining(),0); //
}

TEST_F(BufferTest, ReadWriteUint8) {
    uint8_t value_to_write = 250;
    buffer.write_uint8(value_to_write); //
    ASSERT_EQ(buffer.get_size(), sizeof(uint8_t)); //
    uint8_t read_value = buffer.read_uint8(); //
    ASSERT_EQ(read_value, value_to_write);
    ASSERT_EQ(buffer.get_remaining(), 0); //
}

TEST_F(BufferTest, ReadUint8Underflow) {
    ASSERT_THROW(buffer.read_uint8(), std::runtime_error); //
}

TEST_F(BufferTest, ReadWriteUint64) {
    uint64_t value_to_write = 0x123456789ABCDEF0ULL;
    buffer.write_uint64(value_to_write); //
    ASSERT_EQ(buffer.get_size(), sizeof(uint64_t)); //
    uint64_t read_value = buffer.read_uint64(); //
    ASSERT_EQ(read_value, value_to_write);
    ASSERT_EQ(buffer.get_remaining(), 0); //
}

TEST_F(BufferTest, ReadUint64Underflow) {
    buffer.write_uint32(123); // Write only 4 bytes //
    ASSERT_THROW(buffer.read_uint64(), std::runtime_error); //
}

TEST_F(BufferTest, ReadWriteBytes) {
    const char bytes_to_write[] = "some raw bytes";
    size_t len = std::strlen(bytes_to_write);
    buffer.write_bytes(bytes_to_write, len); //
    ASSERT_EQ(buffer.get_size(), len); //

    std::vector<char> read_bytes_vec(len);
    buffer.read_bytes(read_bytes_vec.data(), len); //
    ASSERT_EQ(std::memcmp(read_bytes_vec.data(), bytes_to_write, len), 0);
    ASSERT_EQ(buffer.get_remaining(), 0); //
}

TEST_F(BufferTest, ReadBytesUnderflow) {
    char temp[10];
    buffer.write_bytes("short", 5); //
    ASSERT_THROW(buffer.read_bytes(temp, 10), std::runtime_error); //
}

TEST_F(BufferTest, ReadBytesZeroLength) {
    char temp[10]; // Should not be written to
    // Fill with a known pattern to ensure it's not modified by a zero-length read
    std::memset(temp, 'A', sizeof(temp));

    buffer.write_bytes("data", 4); // Some data in buffer //
    size_t remaining_before = buffer.get_remaining(); //

    ASSERT_NO_THROW(buffer.read_bytes(temp, 0)); // Reading 0 bytes should not throw //
    ASSERT_EQ(buffer.get_remaining(), remaining_before); // Remaining should be unchanged //

    // Verify that 'temp' was not modified
    for (size_t i = 0; i < sizeof(temp); ++i) {
        ASSERT_EQ(temp[i], 'A');
    }
}


TEST_F(BufferTest, ReadWriteStringEmpty) {
    std::string str_to_write = "";
    buffer.write_string(str_to_write); //
    ASSERT_EQ(buffer.get_size(), sizeof(uint32_t)); // Only length //
    std::string read_str = buffer.read_string(); //
    ASSERT_EQ(read_str, str_to_write);
    ASSERT_TRUE(read_str.empty());
    ASSERT_EQ(buffer.get_remaining(), 0); //
}

TEST_F(BufferTest, ReadWriteStringMaxLength) {
    // Test with a string at MAX_STRING_LENGTH if feasible, or a large string.
    // Be mindful of test execution time and memory for very large strings.
    // For this example, using a moderately large string.
    // std::string str_to_write(netcode::Buffer::MAX_STRING_LENGTH, 'A'); //
    std::string str_to_write(1000, 'A'); // A large, but not max, string for typical testing

    buffer.write_string(str_to_write); //
    ASSERT_EQ(buffer.get_size(), sizeof(uint32_t) + str_to_write.length()); //

    std::string read_str = buffer.read_string(); //
    ASSERT_EQ(read_str, str_to_write);
    ASSERT_EQ(buffer.get_remaining(), 0); //
}

TEST_F(BufferTest, ReadStringTooLongDeclaration) {
    // Write a string length that exceeds MAX_STRING_LENGTH
    uint32_t excessive_length = static_cast<uint32_t>(netcode::Buffer::MAX_STRING_LENGTH + 1); //
    buffer.write_uint32(excessive_length); //
    // Add some dummy bytes, though read_string should throw before trying to read them
    buffer.write_bytes("abc", 3); //

    ASSERT_THROW(buffer.read_string(), std::runtime_error); //
}


TEST_F(BufferTest, SequentialReadWrite) {
    uint8_t u8_val = 10;
    uint32_t u32_val = 2000;
    std::string str_val = "sequence";
    uint64_t u64_val = 3000000000ULL;

    buffer.write_uint8(u8_val); //
    buffer.write_uint32(u32_val); //
    buffer.write_string(str_val); //
    buffer.write_uint64(u64_val); //

    ASSERT_EQ(buffer.read_uint8(), u8_val); //
    ASSERT_EQ(buffer.read_uint32(), u32_val); //
    ASSERT_EQ(buffer.read_string(), str_val); //
    ASSERT_EQ(buffer.read_uint64(), u64_val); //
    ASSERT_EQ(buffer.get_remaining(), 0); //
}

TEST_F(BufferTest, ClearBuffer) {
    buffer.write_uint32(123); //
    buffer.write_string("test"); //
    ASSERT_GT(buffer.get_size(), 0); //

    buffer.clear(); //
    ASSERT_EQ(buffer.get_size(), 0); //
    ASSERT_EQ(buffer.get_remaining(), 0); //
    ASSERT_THROW(buffer.read_uint32(), std::runtime_error); // Should be empty //
}


TEST_F(BufferTest, ConstructorWithData) {
    const char* initial_data_arr = "\x01\x02\x03\x04"; // 4 bytes
    size_t initial_len = 4; // Explicitly use 4, not strlen, for binary data
    netcode::Buffer buf_with_data(initial_data_arr, initial_len); //

    ASSERT_EQ(buf_with_data.get_size(), initial_len); //
    ASSERT_EQ(buf_with_data.get_remaining(), initial_len); //

    ASSERT_EQ(buf_with_data.read_uint8(), 0x01); //
    ASSERT_EQ(buf_with_data.read_uint8(), 0x02); //
    ASSERT_EQ(buf_with_data.read_uint8(), 0x03); //
    ASSERT_EQ(buf_with_data.read_uint8(), 0x04); //
    ASSERT_EQ(buf_with_data.get_remaining(), 0); //

    std::vector<char> initial_vec_data = {'A', 'B', 'C'};
    netcode::Buffer buf_with_vec(initial_vec_data); //
    ASSERT_EQ(buf_with_vec.get_size(), initial_vec_data.size()); //
    char read_char_arr[3];
    buf_with_vec.read_bytes(read_char_arr, 3); //
    ASSERT_EQ(read_char_arr[0], 'A');
    ASSERT_EQ(read_char_arr[1], 'B');
    ASSERT_EQ(read_char_arr[2], 'C');
    ASSERT_EQ(buf_with_vec.get_remaining(), 0); //
}

TEST_F(BufferTest, ReadBytesNullDestination) {
    buffer.write_uint32(123); // Write some data //
    // read_bytes checks for null output buffer if len > 0
    ASSERT_THROW(buffer.read_bytes(nullptr, 1), std::runtime_error); //
    // Reading 0 bytes into nullptr should be fine (no-op)
    ASSERT_NO_THROW(buffer.read_bytes(nullptr, 0)); //
}

// Test for PacketHeader with various MessageType values
TEST_F(BufferTest, ReadWriteHeaderAllMessageTypes) {
    netcode::PacketHeader header_to_write; //
    header_to_write.sequenceNumber = 777; //

    std::vector<netcode::MessageType> types_to_test = { //
        netcode::MessageType::NONE, //
        netcode::MessageType::ECHO_REQUEST, //
        netcode::MessageType::ECHO_RESPONSE, //
        netcode::MessageType::SERVER_ANNOUNCEMENT, //
        // Add other message types from your packet_types.hpp
    };

    for (const auto& msg_type : types_to_test) {
        buffer.clear(); //
        header_to_write.type = msg_type; //

        buffer.write_header(header_to_write); //
        netcode::PacketHeader read_header = buffer.read_header(); //

        ASSERT_EQ(read_header.type, header_to_write.type);
        ASSERT_EQ(read_header.sequenceNumber, header_to_write.sequenceNumber);
        ASSERT_EQ(buffer.get_remaining(), 0); //
    }
}

// Test for ServerAnnouncementData serialization/deserialization
TEST_F(BufferTest, SerializeDeserializeServerAnnouncement) {
    netcode::ServerAnnouncementData data_to_write; //
    data_to_write.message_text = "This is a server announcement!"; //

    netcode::serialize(buffer, data_to_write); //
    ASSERT_GT(buffer.get_size(), 0); //

    netcode::ServerAnnouncementData read_data; //
    bool success = netcode::try_deserialize(buffer, read_data); //

    ASSERT_TRUE(success);
    ASSERT_EQ(read_data.message_text, data_to_write.message_text);
    ASSERT_EQ(buffer.get_remaining(), 0); //
}

TEST_F(BufferTest, TryDeserializeServerAnnouncementFailure) {
    buffer.write_uint32(netcode::Buffer::MAX_STRING_LENGTH + 10); // Invalid length for string //

    netcode::ServerAnnouncementData read_data; //
    bool success = netcode::try_deserialize(buffer, read_data); //
    ASSERT_FALSE(success);
}