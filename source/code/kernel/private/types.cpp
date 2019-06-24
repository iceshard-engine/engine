#include <kernel/types.h>

int32_t readVarintSize(const char* buffer)
{
    int shiftAmount = 0;
    int i = 0;

    do
    {
        shiftAmount += 7;
    } while ((*(buffer + i++) & 0x80) != 0);
    return i;
}

uint64_t readVarint(int32_t& decodedBytes, char* buffer)
{
    uint64_t decodedValue = 0;
    int shiftAmount = 0;
    int i = 0;

    do
    {
        decodedValue |= static_cast<uint64_t>(*(buffer + i) & 0x7F) << shiftAmount;
        shiftAmount += 7;
    } while ((*(buffer + i++) & 0x80) != 0);

    decodedBytes = i;
    return decodedValue;
}

int64_t readSignedVarint(int32_t& decodedBytes, char* buffer)
{
    uint64_t decodedValue = 0;
    int shiftAmount = 0;
    int i = 0;

    do
    {
        decodedValue |= static_cast<uint64_t>(*(buffer + i) & 0x7F) << shiftAmount;
        shiftAmount += 7;
    } while ((*(buffer + i++) & 0x80) != 0);

    decodedBytes = i;

    int64_t result = decodedValue >> 1;
    return decodedValue & 0x1 ? ~result : result; // decodedValue;
}

int writeVarint(uint64_t value, char* buffer)
{
    int encoded = 0;
    do
    {
        uint8_t next_byte = value & 0x7F;
        value >>= 7;

        if (value)
            next_byte |= 0x80;
        *buffer = next_byte;
        ++buffer;
        ++encoded;
    } while (value);
    return encoded;
}

int writeSignedVarint(int64_t value, char* buffer)
{
    int encoded = 0;
    value = (value << 1) ^ (value >> 63);
    do
    {
        uint8_t next_byte = value & 0x7F;
        value >>= 7;

        if (value)
            next_byte |= 0x80;
        *buffer = next_byte;
        ++buffer;
        ++encoded;
    } while (value);
    return encoded;
}
