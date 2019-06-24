#pragma once

#include "kernel.h"

int32 readVarintSize(const char* buffer);

uint64 readVarint(int32& decodedBytes, char* buffer);
int64 readSignedVarint(int32& decodedBytes, char* buffer);

int32 writeVarint(uint64 value, char* buffer);
int32 writeSignedVarint(int64 value, char* buffer);


template<class TYPE>
class varint
{ };


template<>
class varint<uint64>
{
public:
    //
    // Type defines
    //
    using size_t = int32;
    using int_type = uint64;

    //
    // Class interface
    //
    explicit varint(char* buffer) : m_Bytes(readVarintSize(buffer)), m_DataBuffer(MemNew(EMem::Buffer) char[m_Bytes])
    {
        memcpy(m_DataBuffer, buffer, m_Bytes);
    }
    explicit varint()
        : m_Bytes(0), m_DataBuffer(nullptr)
    {

    }
    explicit varint(varint&& that) : m_Bytes(that.m_Bytes), m_DataBuffer(that.m_DataBuffer)
    {
        that.m_DataBuffer = nullptr;
        that.m_Bytes = 0;
    }
    explicit varint(const varint& that) : m_Bytes(that.m_Bytes), m_DataBuffer(MemNew(EMem::Buffer) char[m_Bytes])
    {
        memcpy(m_DataBuffer, that.m_DataBuffer, m_Bytes);
    }
    varint(int_type value)
        : m_Bytes(0), m_DataBuffer(nullptr)
    {
        char buffer[10];
        m_Bytes = writeVarint(value, buffer);

        if (m_DataBuffer) delete[] m_DataBuffer;
        m_DataBuffer = MemNew(EMem::Buffer) char[m_Bytes];
        memcpy(m_DataBuffer, buffer, m_Bytes);
    }
    ~varint()
    {
        delete[] m_DataBuffer;
        m_DataBuffer = nullptr;
        m_Bytes = 0;
    }

    void operator=(int_type value)
    {
        char buffer[10];
        m_Bytes = writeVarint(value, buffer);

        if (m_DataBuffer) delete[] m_DataBuffer;
        m_DataBuffer = MemNew(EMem::Buffer) char[m_Bytes];
        memcpy(m_DataBuffer, buffer, m_Bytes);
    }
    void operator=(varint&& that)
    {
        m_Bytes = that.m_Bytes;
        m_DataBuffer = that.m_DataBuffer;

        that.m_DataBuffer = nullptr;
        that.m_Bytes = 0;
    }

    int_type getValue() const
    {
        size_t bytes;
        return readVarint(bytes, m_DataBuffer);
    }

    operator int_type() const
    {
        return getValue();
    }

    size_t getByteSize() const
    {
        return m_Bytes;
    }
    char* getDataBuffer() const
    {
        return m_DataBuffer;
    }

private:
    size_t m_Bytes;
    char* m_DataBuffer;
};



template<>
class varint<int64>
{
public:
    //
    // Type defines
    //
    using size_t = int32;
    using int_type = int64;

    //
    // Class interface
    //
    explicit varint(const char* buffer)
        : m_Bytes(readVarintSize(buffer)), m_DataBuffer(MemNew(EMem::Buffer) char[m_Bytes])
    {
        memcpy(m_DataBuffer, buffer, m_Bytes);
    }
    explicit varint()
        : m_Bytes(0), m_DataBuffer(nullptr)
    {

    }
    explicit varint(varint&& that)
        : m_Bytes(that.m_Bytes), m_DataBuffer(that.m_DataBuffer)
    {
        that.m_DataBuffer = nullptr;
        that.m_Bytes = 0;
    }
    explicit varint(const varint& that) : m_Bytes(that.m_Bytes), m_DataBuffer(MemNew(EMem::Buffer) char[m_Bytes])
    {
        memcpy(m_DataBuffer, that.m_DataBuffer, m_Bytes);
    }
    varint(int_type value)
        : m_Bytes(0), m_DataBuffer(nullptr)
    {
        char buffer[10];
        m_Bytes = writeSignedVarint(value, buffer);

        if (m_DataBuffer) delete[] m_DataBuffer;
        m_DataBuffer = new char[m_Bytes];
        memcpy(m_DataBuffer, buffer, m_Bytes);
    }
    ~varint()
    {
        delete[] m_DataBuffer;
    }

    void operator=(int_type value)
    {
        char buffer[10];
        m_Bytes = writeSignedVarint(value, buffer);

        if (m_DataBuffer) delete[] m_DataBuffer;
        m_DataBuffer = MemNew(EMem::Buffer) char[m_Bytes];
        memcpy(m_DataBuffer, buffer, m_Bytes);
    }
    void operator=(varint&& that)
    {
        m_Bytes = that.m_Bytes;
        m_DataBuffer = that.m_DataBuffer;

        that.m_DataBuffer = nullptr;
        that.m_Bytes = 0;
    }

    int_type getValue() const
    {
        size_t bytes;
        return readSignedVarint(bytes, m_DataBuffer);
    }

    operator int_type() const
    {
        return getValue();
    }

    size_t getByteSize() const
    {
        return m_Bytes;
    }
    char* getDataBuffer() const
    {
        return m_DataBuffer;
    }

private:
    size_t m_Bytes;
    char* m_DataBuffer;
};

using uvarint = varint<uint64>;
using svarint = varint<int64>;
