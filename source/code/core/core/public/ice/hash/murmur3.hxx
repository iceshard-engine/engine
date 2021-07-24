//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

// Note - The x86 and x64 versions do _not_ produce the same results, as the
// algorithms are optimized for their respective platforms. You can still
// compile and run any of them on any platform, but your performance with the
// non-native version will be less than optimal.

#pragma once
#include <cinttypes>
#include <string_view>

namespace ice::detail::murmur3_hash
{

    struct mm3_x86_h32
    {
        uint32_t h[1];
    };

    struct mm3_x86_h128
    {
        uint32_t h[4];
    };

    struct mm3_x64_h128
    {
        uint64_t h[2];
    };

    constexpr auto cexpr_murmur3_x86_32(std::string_view key, uint32_t seed) noexcept -> mm3_x86_h32;
    constexpr auto cexpr_murmur3_x86_128(std::string_view key, uint32_t seed) noexcept -> mm3_x86_h128;
    constexpr auto cexpr_murmur3_x64_128(std::string_view key, uint32_t seed) noexcept -> mm3_x64_h128;

    namespace detail
    {

        constexpr uint32_t cexpr_rotl32(uint32_t x, int8_t r) noexcept
        {
            return (x << r) | (x >> (32 - r));
        }

        constexpr uint64_t cexpr_rotl64(uint64_t x, int8_t r) noexcept
        {
            return (x << r) | (x >> (64 - r));
        }

        constexpr auto cexpr_block_x32(char const* data) noexcept -> uint32_t
        {
            uint32_t result = 0;
            result |= static_cast<uint8_t const>(data[3]);
            result <<= 8;
            result |= static_cast<uint8_t const>(data[2]);
            result <<= 8;
            result |= static_cast<uint8_t const>(data[1]);
            result <<= 8;
            result |= static_cast<uint8_t const>(data[0]);
            return result;
        }

        constexpr auto cexpr_block_x64(char const* data) noexcept -> uint64_t
        {
            uint64_t result = 0;
            result |= static_cast<uint8_t const>(data[7]);
            result <<= 8;
            result |= static_cast<uint8_t const>(data[6]);
            result <<= 8;
            result |= static_cast<uint8_t const>(data[5]);
            result <<= 8;
            result |= static_cast<uint8_t const>(data[4]);
            result <<= 8;
            result |= static_cast<uint8_t const>(data[3]);
            result <<= 8;
            result |= static_cast<uint8_t const>(data[2]);
            result <<= 8;
            result |= static_cast<uint8_t const>(data[1]);
            result <<= 8;
            result |= static_cast<uint8_t const>(data[0]);
            return result;
        }

        //-----------------------------------------------------------------------------
        // Finalization mix - force all bits of a hash block to avalanche

        constexpr uint32_t cexpr_fmix32(uint32_t h) noexcept
        {
            h ^= h >> 16;
            h *= 0x85ebca6b;
            h ^= h >> 13;
            h *= 0xc2b2ae35;
            h ^= h >> 16;
            return h;
        }

        //----------

        constexpr uint64_t cexpr_fmix64(uint64_t k) noexcept
        {
            k ^= k >> 33;
            k *= 0xFF51AFD7ED558CCDllu;
            k ^= k >> 33;
            k *= 0xC4CEB9FE1A85EC53llu;
            k ^= k >> 33;
            return k;
        }

        //-----------------------------------------------------------------------------

        constexpr auto cexpr_murmur3_x86_32(std::string_view key, uint32_t seed) noexcept -> mm3_x86_h32
        {
            char const* string_data = key.data();
            uint32_t const string_length = key.length();

            uint32_t const block_byte_size = 4u;
            uint32_t const block_num = string_length / block_byte_size;

            //----------
            // body

            uint32_t const const_1 = 0xcc9e2d51;
            uint32_t const const_2 = 0x1b873593;
            uint32_t hash_r1 = seed;

            char const* blocks_end = string_data + static_cast<uintptr_t>(block_num) * block_byte_size;

            for (size_t idx = block_num; idx > 0; --idx)
            {
                uint32_t k1 = cexpr_block_x32(blocks_end - (idx * block_byte_size));

                k1 = cexpr_rotl32(k1 * const_1, 15) * const_2;

                hash_r1 ^= k1;

                hash_r1 = cexpr_rotl32(hash_r1, 13) * 5 + 0xe6546b64;
            }

            //----------
            // tail

            char const* tail = blocks_end;

            uint32_t k1 = 0;

            switch (string_length & 3)
            {
            case 3:
                k1 ^= static_cast<uint8_t const>(tail[2]) << 16;
                [[fallthrough]];
            case 2:
                k1 ^= static_cast<uint8_t const>(tail[1]) << 8;
                [[fallthrough]];
            case 1:
                k1 ^= static_cast<uint8_t const>(tail[0]);
            };

            k1 = cexpr_rotl32(k1 * const_1, 15) * const_2;
            hash_r1 ^= k1;

            //----------
            // finalization

            hash_r1 ^= static_cast<uint32_t>(string_length);

            hash_r1 = cexpr_fmix32(hash_r1);

            return mm3_x86_h32{ .h = { hash_r1 } };
        }

        //-----------------------------------------------------------------------------

        constexpr auto cexpr_murmur3_x86_128(std::string_view key, uint32_t seed) noexcept -> mm3_x86_h128
        {
            char const* string_data = key.data();
            uint32_t const string_length = key.length();

            uint32_t const block_byte_size = 16u;
            uint32_t const block_num = string_length / block_byte_size;

            //----------
            // body

            uint32_t hash_r1 = seed;
            uint32_t hash_r2 = seed;
            uint32_t hash_r3 = seed;
            uint32_t hash_r4 = seed;

            uint32_t const const_1 = 0x239b961b;
            uint32_t const const_2 = 0xab0e9789;
            uint32_t const const_3 = 0x38b34ae5;
            uint32_t const const_4 = 0xa1e38b93;

            char const* blocks_end = string_data + static_cast<uintptr_t>(block_num) * block_byte_size;

            for (size_t idx = block_num; idx > 0; --idx)
            {
                uint32_t k1 = cexpr_block_x32(blocks_end - (idx * block_byte_size - 0));
                uint32_t k2 = cexpr_block_x32(blocks_end - (idx * block_byte_size - 4));
                uint32_t k3 = cexpr_block_x32(blocks_end - (idx * block_byte_size - 8));
                uint32_t k4 = cexpr_block_x32(blocks_end - (idx * block_byte_size - 12));

                k1 = cexpr_rotl32(k1 * const_1, 15) * const_2;
                hash_r1 ^= k1;

                hash_r1 = cexpr_rotl32(hash_r1, 19) + hash_r2;
                hash_r1 = hash_r1 * 5 + 0x561ccd1b;

                k2 = cexpr_rotl32(k2 * const_2, 16) * const_3;
                hash_r2 ^= k2;

                hash_r2 = cexpr_rotl32(hash_r2, 17) + hash_r3;
                hash_r2 = hash_r2 * 5 + 0x0bcaa747;

                k3 = cexpr_rotl32(k3 * const_3, 17) * const_4;
                hash_r3 ^= k3;

                hash_r3 = cexpr_rotl32(hash_r3, 15) + hash_r4;
                hash_r3 = hash_r3 * 5 + 0x96cd1c35;

                k4 = cexpr_rotl32(k4 * const_4, 18) * const_1;
                hash_r4 ^= k4;

                hash_r4 = cexpr_rotl32(hash_r4, 13) + hash_r1;
                hash_r4 = hash_r4 * 5 + 0x32ac3b17;
            }

            //----------
            // tail

            char const* tail = blocks_end;

            uint32_t k1 = 0;
            uint32_t k2 = 0;
            uint32_t k3 = 0;
            uint32_t k4 = 0;

            switch (string_length & 15)
            {
            case 15: k4 ^= static_cast<uint8_t const>(tail[14]) << 16;
                [[fallthrough]];
            case 14: k4 ^= static_cast<uint8_t const>(tail[13]) << 8;
                [[fallthrough]];
            case 13: k4 ^= static_cast<uint8_t const>(tail[12]) << 0;
                k4 = cexpr_rotl32(k4 * const_4, 18) * const_1;
                hash_r4 ^= k4;
                [[fallthrough]];

            case 12: k3 ^= static_cast<uint8_t const>(tail[11]) << 24;
                [[fallthrough]];
            case 11: k3 ^= static_cast<uint8_t const>(tail[10]) << 16;
                [[fallthrough]];
            case 10: k3 ^= static_cast<uint8_t const>(tail[9]) << 8;
                [[fallthrough]];
            case  9: k3 ^= static_cast<uint8_t const>(tail[8]) << 0;
                k3 = cexpr_rotl32(k3 * const_3, 17) * const_4;
                hash_r3 ^= k3;
                [[fallthrough]];

            case  8: k2 ^= static_cast<uint8_t const>(tail[7]) << 24;
                [[fallthrough]];
            case  7: k2 ^= static_cast<uint8_t const>(tail[6]) << 16;
                [[fallthrough]];
            case  6: k2 ^= static_cast<uint8_t const>(tail[5]) << 8;
                [[fallthrough]];
            case  5: k2 ^= static_cast<uint8_t const>(tail[4]) << 0;
                k2 = cexpr_rotl32(k2 * const_2, 16) * const_3;
                hash_r2 ^= k2;
                [[fallthrough]];

            case  4: k1 ^= static_cast<uint8_t const>(tail[3]) << 24;
                [[fallthrough]];
            case  3: k1 ^= static_cast<uint8_t const>(tail[2]) << 16;
                [[fallthrough]];
            case  2: k1 ^= static_cast<uint8_t const>(tail[1]) << 8;
                [[fallthrough]];
            case  1: k1 ^= static_cast<uint8_t const>(tail[0]) << 0;
                k1 = cexpr_rotl32(k1 * const_1, 15) * const_2;
                hash_r1 ^= k1;
            };

            //----------
            // finalization

            hash_r1 ^= string_length;
            hash_r2 ^= string_length;
            hash_r3 ^= string_length;
            hash_r4 ^= string_length;

            hash_r1 += hash_r2;
            hash_r1 += hash_r3;
            hash_r1 += hash_r4;
            hash_r2 += hash_r1;
            hash_r3 += hash_r1;
            hash_r4 += hash_r1;

            hash_r1 = cexpr_fmix32(hash_r1);
            hash_r2 = cexpr_fmix32(hash_r2);
            hash_r3 = cexpr_fmix32(hash_r3);
            hash_r4 = cexpr_fmix32(hash_r4);

            hash_r1 += hash_r2;
            hash_r1 += hash_r3;
            hash_r1 += hash_r4;
            hash_r2 += hash_r1;
            hash_r3 += hash_r1;
            hash_r4 += hash_r1;

            return mm3_x86_h128{ .h = { hash_r1, hash_r2, hash_r3, hash_r4 } };
        }

        //-----------------------------------------------------------------------------

        constexpr auto cexpr_murmur3_x64_128(std::string_view key, uint32_t seed) noexcept -> mm3_x64_h128
        {
            char const* string_data = key.data();
            uint64_t const string_length = key.length();

            uint64_t const block_byte_size = 16u;
            uint64_t const block_num = string_length / block_byte_size;

            //----------
            // body

            uint64_t hash_r1 = seed;
            uint64_t hash_r2 = seed;

            uint64_t const const_1 = 0x87c37b91114253d5llu;
            uint64_t const const_2 = 0x4cf5ad432745937fllu;

            char const* blocks_beg = string_data;

            for (uint32_t idx = 0; idx < block_num; ++idx)
            {
                uint64_t k1 = cexpr_block_x64(blocks_beg + (idx * block_byte_size + 0));
                uint64_t k2 = cexpr_block_x64(blocks_beg + (idx * block_byte_size + 8));

                k1 = cexpr_rotl64(k1 * const_1, 31) * const_2;
                hash_r1 ^= k1;

                hash_r1 = cexpr_rotl64(hash_r1, 27) + hash_r2;
                hash_r1 = hash_r1 * 5 + 0x52dce729;

                k2 = cexpr_rotl64(k2 * const_2, 33) * const_1;
                hash_r2 ^= k2;

                hash_r2 = cexpr_rotl64(hash_r2, 31) + hash_r1;
                hash_r2 = hash_r2 * 5 + 0x38495ab5;
            }

            //----------
            // tail

            char const* tail = blocks_beg + block_num * block_byte_size;

            uint64_t k1 = 0;
            uint64_t k2 = 0;

            switch (string_length & 15)
            {
            case 15: k2 ^= static_cast<uint64_t const>(static_cast<uint8_t const>(tail[14])) << 48;
                [[fallthrough]];
            case 14: k2 ^= static_cast<uint64_t const>(static_cast<uint8_t const>(tail[13])) << 40;
                [[fallthrough]];
            case 13: k2 ^= static_cast<uint64_t const>(static_cast<uint8_t const>(tail[12])) << 32;
                [[fallthrough]];
            case 12: k2 ^= static_cast<uint64_t const>(static_cast<uint8_t const>(tail[11])) << 24;
                [[fallthrough]];
            case 11: k2 ^= static_cast<uint64_t const>(static_cast<uint8_t const>(tail[10])) << 16;
                [[fallthrough]];
            case 10: k2 ^= static_cast<uint64_t const>(static_cast<uint8_t const>(tail[9])) << 8;
                [[fallthrough]];
            case  9: k2 ^= static_cast<uint64_t const>(static_cast<uint8_t const>(tail[8])) << 0;
                k2 = cexpr_rotl64(k2 * const_2, 33) * const_1;
                hash_r2 ^= k2;
                [[fallthrough]];

            case  8: k1 ^= static_cast<uint64_t const>(static_cast<uint8_t const>(tail[7])) << 56;
                [[fallthrough]];
            case  7: k1 ^= static_cast<uint64_t const>(static_cast<uint8_t const>(tail[6])) << 48;
                [[fallthrough]];
            case  6: k1 ^= static_cast<uint64_t const>(static_cast<uint8_t const>(tail[5])) << 40;
                [[fallthrough]];
            case  5: k1 ^= static_cast<uint64_t const>(static_cast<uint8_t const>(tail[4])) << 32;
                [[fallthrough]];
            case  4: k1 ^= static_cast<uint64_t const>(static_cast<uint8_t const>(tail[3])) << 24;
                [[fallthrough]];
            case  3: k1 ^= static_cast<uint64_t const>(static_cast<uint8_t const>(tail[2])) << 16;
                [[fallthrough]];
            case  2: k1 ^= static_cast<uint64_t const>(static_cast<uint8_t const>(tail[1])) << 8;
                [[fallthrough]];
            case  1: k1 ^= static_cast<uint64_t const>(static_cast<uint8_t const>(tail[0])) << 0;
                k1 = cexpr_rotl64(k1 * const_1, 31) * const_2;
                hash_r1 ^= k1;
            };

            //----------
            // finalization

            hash_r1 ^= string_length;
            hash_r2 ^= string_length;

            hash_r1 += hash_r2;
            hash_r2 += hash_r1;

            hash_r1 = cexpr_fmix64(hash_r1);
            hash_r2 = cexpr_fmix64(hash_r2);

            hash_r1 += hash_r2;
            hash_r2 += hash_r1;

            return mm3_x64_h128{ .h = { hash_r1, hash_r2 } };
        }

    } // namespace detail

    constexpr auto cexpr_murmur3_x86_32(std::string_view key, uint32_t seed) noexcept -> mm3_x86_h32
    {
        return detail::cexpr_murmur3_x86_32(key, seed);
    }

    constexpr auto cexpr_murmur3_x86_128(std::string_view key, uint32_t seed) noexcept -> mm3_x86_h128
    {
        return detail::cexpr_murmur3_x86_128(key, seed);
    }

    constexpr auto cexpr_murmur3_x64_128(std::string_view key, uint32_t seed) noexcept -> mm3_x64_h128
    {
        return detail::cexpr_murmur3_x64_128(key, seed);
    }

} // namespace ice::detail::murmur3_hash
