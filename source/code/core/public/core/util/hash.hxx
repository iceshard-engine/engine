#pragma once
#include <kernel/types.h>

/* The following code was copied from https://gist.github.com/Teknoman117/d4d952942b4314781432 */

namespace compiletime
{
    namespace internal
    {
        // Murmur hash constants
        constexpr const uint64_t m = 0xc6a4a7935bd1e995;
        constexpr const int      r = 47;

        constexpr uint64_t crotate(uint64_t a)
        {
            return a ^ (a >> r);
        }

        constexpr uint64_t cfinalize_h(const char *data, size_t key, uint64_t h)
        {
            return (key != 0) ? cfinalize_h(data, key - 1, (h ^ (uint64_t(data[key - 1]) << (8 * (key - 1))))) : h * m;
        }

        constexpr uint64_t cfinalize(const char *data, size_t len, uint64_t h)
        {
            return (len & 7) ? crotate(crotate(cfinalize_h(data, len & 7, h)) * m)
                : crotate(crotate(h) * m);
        }

        // reinterpret cast is illegal (static is fine) so we have to manually load 64 bit chuncks of string instead
        // of casting char* to uint64_t*
        //
        // TODO - this only works on little endian machines .... fuuuu
        constexpr uint64_t cblock(const char *data, size_t offset = 0)
        {
            return (offset == 7) ? uint64_t(data[offset]) << (8 * offset)
                : (uint64_t(data[offset]) << (8 * offset)) | cblock(data, offset + 1);
        }

        // Mixing function for the hash function
        constexpr uint64_t cmix_h(const char *data, uint64_t h, size_t offset)
        {
            return (h ^ (crotate(cblock(data + offset) * m) * m)) * m;
        }

        // Control function for the mixing
        constexpr uint64_t cmix(const char *data, size_t len, uint64_t h, size_t offset = 0)
        {
            return (offset == (len & ~size_t(7))) ? cfinalize(data + offset, len, h)
                : cmix(data, len, cmix_h(data, h, offset), offset + 8);
        }
    }

    // Compile time version of strlen
    constexpr size_t length_cstring(const char *data, const size_t idx = 0)
    {
        return (data[idx] == '\0') ? idx : length_cstring(data, idx + 1);
    }

    // Compile time version of MurmurHash2 for
    constexpr uint64_t hash_cstring(const char *data, size_t len, uint64_t seed = 0xc70f6907UL)
    {
        return internal::cmix(data, len, seed ^ (len * internal::m));
    }
}
