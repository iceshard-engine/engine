/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

// Note - The x86 and x64 versions do _not_ produce the same results, as the
// algorithms are optimized for their respective platforms. You can still
// compile and run any of them on any platform, but your performance with the
// non-native version will be less than optimal.

#pragma once
#include <ice/types.hxx>
#include <string_view>

namespace ice::detail::murmur3_hash
{

    struct mm3_x86_h32
    {
        ice::u32 h[1];
    };

    struct mm3_x86_h128
    {
        ice::u32 h[4];
    };

    struct mm3_x64_h128
    {
        ice::u64 h[2];
    };

    constexpr auto cexpr_murmur3_x86_32(std::u8string_view key, ice::u32 seed) noexcept -> mm3_x86_h32;
    constexpr auto cexpr_murmur3_x86_128(std::u8string_view key, ice::u32 seed) noexcept -> mm3_x86_h128;
    constexpr auto cexpr_murmur3_x64_128(std::u8string_view key, ice::u32 seed) noexcept -> mm3_x64_h128;

    constexpr auto cexpr_murmur3_x86_32(std::string_view key, ice::u32 seed) noexcept -> mm3_x86_h32;
    constexpr auto cexpr_murmur3_x86_128(std::string_view key, ice::u32 seed) noexcept -> mm3_x86_h128;
    constexpr auto cexpr_murmur3_x64_128(std::string_view key, ice::u32 seed) noexcept -> mm3_x64_h128;

    namespace detail
    {

        constexpr auto cexpr_rotl32(ice::u32 x, ice::i8 r) noexcept -> ice::u32
        {
            return (x << r) | (x >> (32 - r));
        }

        constexpr auto cexpr_rotl64(ice::u64 x, ice::i8 r) noexcept -> ice::u64
        {
            return (x << r) | (x >> (64 - r));
        }

        template<typename Char>
        constexpr auto cexpr_block_x32(Char const* data) noexcept -> ice::u32
        {
            ice::u32 result = 0;
            result |= static_cast<ice::u8 const>(data[3]);
            result <<= 8;
            result |= static_cast<ice::u8 const>(data[2]);
            result <<= 8;
            result |= static_cast<ice::u8 const>(data[1]);
            result <<= 8;
            result |= static_cast<ice::u8 const>(data[0]);
            return result;
        }

        template<typename Char>
        constexpr auto cexpr_block_x64(Char const* data) noexcept -> ice::u64
        {
            ice::u64 result = 0;
            result |= static_cast<ice::u8 const>(data[7]);
            result <<= 8;
            result |= static_cast<ice::u8 const>(data[6]);
            result <<= 8;
            result |= static_cast<ice::u8 const>(data[5]);
            result <<= 8;
            result |= static_cast<ice::u8 const>(data[4]);
            result <<= 8;
            result |= static_cast<ice::u8 const>(data[3]);
            result <<= 8;
            result |= static_cast<ice::u8 const>(data[2]);
            result <<= 8;
            result |= static_cast<ice::u8 const>(data[1]);
            result <<= 8;
            result |= static_cast<ice::u8 const>(data[0]);
            return result;
        }

        //-----------------------------------------------------------------------------
        // Finalization mix - force all bits of a hash block to avalanche

        constexpr auto cexpr_fmix32(ice::u32 h) noexcept -> ice::u32
        {
            h ^= h >> 16;
            h *= 0x85ebca6b;
            h ^= h >> 13;
            h *= 0xc2b2ae35;
            h ^= h >> 16;
            return h;
        }

        //----------

        constexpr ice::u64 cexpr_fmix64(ice::u64 k) noexcept
        {
            k ^= k >> 33;
            k *= 0xFF51AFD7ED558CCDllu;
            k ^= k >> 33;
            k *= 0xC4CEB9FE1A85EC53llu;
            k ^= k >> 33;
            return k;
        }

        //-----------------------------------------------------------------------------

        template<typename Char>
        constexpr auto cexpr_murmur3_x86_32(std::basic_string_view<Char> key, ice::u32 seed) noexcept -> mm3_x86_h32
        {
            Char const* string_data = key.data();
            ice::u32 const string_length = static_cast<ice::u32>(key.length());

            ice::u32 const block_byte_size = 4u;
            ice::u32 const block_num = string_length / block_byte_size;

            //----------
            // body

            ice::u32 const const_1 = 0xcc9e2d51;
            ice::u32 const const_2 = 0x1b873593;
            ice::u32 hash_r1 = seed;

            Char const* blocks_end = string_data + static_cast<ice::uptr>(block_num) * block_byte_size;

            for (size_t idx = block_num; idx > 0; --idx)
            {
                ice::u32 k1 = cexpr_block_x32(blocks_end - (idx * block_byte_size));

                k1 = cexpr_rotl32(k1 * const_1, 15) * const_2;

                hash_r1 ^= k1;

                hash_r1 = cexpr_rotl32(hash_r1, 13) * 5 + 0xe6546b64;
            }

            //----------
            // tail

            Char const* tail = blocks_end;

            ice::u32 k1 = 0;

            switch (string_length & 3)
            {
            case 3:
                k1 ^= static_cast<ice::u8 const>(tail[2]) << 16;
                [[fallthrough]];
            case 2:
                k1 ^= static_cast<ice::u8 const>(tail[1]) << 8;
                [[fallthrough]];
            case 1:
                k1 ^= static_cast<ice::u8 const>(tail[0]);
            };

            k1 = cexpr_rotl32(k1 * const_1, 15) * const_2;
            hash_r1 ^= k1;

            //----------
            // finalization

            hash_r1 ^= static_cast<ice::u32>(string_length);

            hash_r1 = cexpr_fmix32(hash_r1);

            return mm3_x86_h32{ .h = { hash_r1 } };
        }

        //-----------------------------------------------------------------------------

        template<typename Char>
        constexpr auto cexpr_murmur3_x86_128(std::basic_string_view<Char> key, ice::u32 seed) noexcept -> mm3_x86_h128
        {
            Char const* string_data = key.data();
            ice::u32 const string_length = static_cast<ice::u32>(key.length());

            ice::u32 const block_byte_size = 16u;
            ice::u32 const block_num = string_length / block_byte_size;

            //----------
            // body

            ice::u32 hash_r1 = seed;
            ice::u32 hash_r2 = seed;
            ice::u32 hash_r3 = seed;
            ice::u32 hash_r4 = seed;

            ice::u32 const const_1 = 0x239b961b;
            ice::u32 const const_2 = 0xab0e9789;
            ice::u32 const const_3 = 0x38b34ae5;
            ice::u32 const const_4 = 0xa1e38b93;

            Char const* blocks_end = string_data + static_cast<ice::uptr>(block_num) * block_byte_size;

            for (size_t idx = block_num; idx > 0; --idx)
            {
                ice::u32 k1 = cexpr_block_x32(blocks_end - (idx * block_byte_size - 0));
                ice::u32 k2 = cexpr_block_x32(blocks_end - (idx * block_byte_size - 4));
                ice::u32 k3 = cexpr_block_x32(blocks_end - (idx * block_byte_size - 8));
                ice::u32 k4 = cexpr_block_x32(blocks_end - (idx * block_byte_size - 12));

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

            Char const* tail = blocks_end;

            ice::u32 k1 = 0;
            ice::u32 k2 = 0;
            ice::u32 k3 = 0;
            ice::u32 k4 = 0;

            switch (string_length & 15)
            {
            case 15: k4 ^= static_cast<ice::u8 const>(tail[14]) << 16;
                [[fallthrough]];
            case 14: k4 ^= static_cast<ice::u8 const>(tail[13]) << 8;
                [[fallthrough]];
            case 13: k4 ^= static_cast<ice::u8 const>(tail[12]) << 0;
                k4 = cexpr_rotl32(k4 * const_4, 18) * const_1;
                hash_r4 ^= k4;
                [[fallthrough]];

            case 12: k3 ^= static_cast<ice::u8 const>(tail[11]) << 24;
                [[fallthrough]];
            case 11: k3 ^= static_cast<ice::u8 const>(tail[10]) << 16;
                [[fallthrough]];
            case 10: k3 ^= static_cast<ice::u8 const>(tail[9]) << 8;
                [[fallthrough]];
            case  9: k3 ^= static_cast<ice::u8 const>(tail[8]) << 0;
                k3 = cexpr_rotl32(k3 * const_3, 17) * const_4;
                hash_r3 ^= k3;
                [[fallthrough]];

            case  8: k2 ^= static_cast<ice::u8 const>(tail[7]) << 24;
                [[fallthrough]];
            case  7: k2 ^= static_cast<ice::u8 const>(tail[6]) << 16;
                [[fallthrough]];
            case  6: k2 ^= static_cast<ice::u8 const>(tail[5]) << 8;
                [[fallthrough]];
            case  5: k2 ^= static_cast<ice::u8 const>(tail[4]) << 0;
                k2 = cexpr_rotl32(k2 * const_2, 16) * const_3;
                hash_r2 ^= k2;
                [[fallthrough]];

            case  4: k1 ^= static_cast<ice::u8 const>(tail[3]) << 24;
                [[fallthrough]];
            case  3: k1 ^= static_cast<ice::u8 const>(tail[2]) << 16;
                [[fallthrough]];
            case  2: k1 ^= static_cast<ice::u8 const>(tail[1]) << 8;
                [[fallthrough]];
            case  1: k1 ^= static_cast<ice::u8 const>(tail[0]) << 0;
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

        template<typename Char>
        constexpr auto cexpr_murmur3_x64_128(std::basic_string_view<Char> key, ice::u32 seed) noexcept -> mm3_x64_h128
        {
            Char const* string_data = key.data();
            ice::u64 const string_length = key.length();

            ice::u64 const block_byte_size = 16u;
            ice::u64 const block_num = string_length / block_byte_size;

            //----------
            // body

            ice::u64 hash_r1 = seed;
            ice::u64 hash_r2 = seed;

            ice::u64 const const_1 = 0x87c37b91114253d5llu;
            ice::u64 const const_2 = 0x4cf5ad432745937fllu;

            Char const* blocks_beg = string_data;

            for (ice::u32 idx = 0; idx < block_num; ++idx)
            {
                ice::u64 k1 = cexpr_block_x64(blocks_beg + (idx * block_byte_size + 0));
                ice::u64 k2 = cexpr_block_x64(blocks_beg + (idx * block_byte_size + 8));

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

            Char const* tail = blocks_beg + block_num * block_byte_size;

            ice::u64 k1 = 0;
            ice::u64 k2 = 0;

            switch (string_length & 15)
            {
            case 15: k2 ^= static_cast<ice::u64 const>(static_cast<ice::u8 const>(tail[14])) << 48;
                [[fallthrough]];
            case 14: k2 ^= static_cast<ice::u64 const>(static_cast<ice::u8 const>(tail[13])) << 40;
                [[fallthrough]];
            case 13: k2 ^= static_cast<ice::u64 const>(static_cast<ice::u8 const>(tail[12])) << 32;
                [[fallthrough]];
            case 12: k2 ^= static_cast<ice::u64 const>(static_cast<ice::u8 const>(tail[11])) << 24;
                [[fallthrough]];
            case 11: k2 ^= static_cast<ice::u64 const>(static_cast<ice::u8 const>(tail[10])) << 16;
                [[fallthrough]];
            case 10: k2 ^= static_cast<ice::u64 const>(static_cast<ice::u8 const>(tail[9])) << 8;
                [[fallthrough]];
            case  9: k2 ^= static_cast<ice::u64 const>(static_cast<ice::u8 const>(tail[8])) << 0;
                k2 = cexpr_rotl64(k2 * const_2, 33) * const_1;
                hash_r2 ^= k2;
                [[fallthrough]];

            case  8: k1 ^= static_cast<ice::u64 const>(static_cast<ice::u8 const>(tail[7])) << 56;
                [[fallthrough]];
            case  7: k1 ^= static_cast<ice::u64 const>(static_cast<ice::u8 const>(tail[6])) << 48;
                [[fallthrough]];
            case  6: k1 ^= static_cast<ice::u64 const>(static_cast<ice::u8 const>(tail[5])) << 40;
                [[fallthrough]];
            case  5: k1 ^= static_cast<ice::u64 const>(static_cast<ice::u8 const>(tail[4])) << 32;
                [[fallthrough]];
            case  4: k1 ^= static_cast<ice::u64 const>(static_cast<ice::u8 const>(tail[3])) << 24;
                [[fallthrough]];
            case  3: k1 ^= static_cast<ice::u64 const>(static_cast<ice::u8 const>(tail[2])) << 16;
                [[fallthrough]];
            case  2: k1 ^= static_cast<ice::u64 const>(static_cast<ice::u8 const>(tail[1])) << 8;
                [[fallthrough]];
            case  1: k1 ^= static_cast<ice::u64 const>(static_cast<ice::u8 const>(tail[0])) << 0;
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

    constexpr auto cexpr_murmur3_x86_32(std::u8string_view key, ice::u32 seed) noexcept -> mm3_x86_h32
    {
        return detail::cexpr_murmur3_x86_32<ice::utf8>(key, seed);
    }

    constexpr auto cexpr_murmur3_x86_128(std::u8string_view key, ice::u32 seed) noexcept -> mm3_x86_h128
    {
        return detail::cexpr_murmur3_x86_128<ice::utf8>(key, seed);
    }

    constexpr auto cexpr_murmur3_x64_128(std::u8string_view key, ice::u32 seed) noexcept -> mm3_x64_h128
    {
        return detail::cexpr_murmur3_x64_128<ice::utf8>(key, seed);
    }

    constexpr auto cexpr_murmur3_x86_32(std::string_view key, ice::u32 seed) noexcept -> mm3_x86_h32
    {
        return detail::cexpr_murmur3_x86_32<char>(key, seed);
    }

    constexpr auto cexpr_murmur3_x86_128(std::string_view key, ice::u32 seed) noexcept -> mm3_x86_h128
    {
        return detail::cexpr_murmur3_x86_128<char>(key, seed);
    }

    constexpr auto cexpr_murmur3_x64_128(std::string_view key, ice::u32 seed) noexcept -> mm3_x64_h128
    {
        return detail::cexpr_murmur3_x64_128<char>(key, seed);
    }

} // namespace ice::detail::murmur3_hash
