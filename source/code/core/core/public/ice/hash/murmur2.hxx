//!
//! The below code was taken from https://gist.github.com/Teknoman117/d4d952942b4314781432
//! It is used by the stringid_t type to calculate the hash value from the given string at compile time.
//!
/*
 * Copyright (c) 2018 Nathan Lewis <linux.robotdude@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of mosquitto nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include <cinttypes>
#include <string_view>

namespace ice::detail::murmur2_hash
{

    struct mm2_x64_64
    {
        uint64_t h[1];
    };

    constexpr auto cexpr_murmur2_x64_64(std::string_view key, uint64_t seed) noexcept -> mm2_x64_64;

    namespace detail
    {

        // Murmur hash constants
        constexpr uint64_t  m = 0xc6a4a7935bd1e995;
        constexpr int       r = 47;

        constexpr uint64_t crotate(uint64_t a) noexcept
        {
            return a ^ (a >> r);
        }

        constexpr uint64_t cfinalize_h(const char *data, size_t key, uint64_t h) noexcept
        {
            return (key != 0) ? cfinalize_h(data, key - 1, (h ^ (uint64_t(data[key - 1]) << (8 * (key - 1))))) : h * m;
        }

        constexpr uint64_t cfinalize(const char *data, size_t len, uint64_t h) noexcept
        {
            return (len & 7) ? crotate(crotate(cfinalize_h(data, len & 7, h)) * m)
                : crotate(crotate(h) * m);
        }

        // reinterpret cast is illegal (static is fine) so we have to manually load 64 bit chuncks of string instead
        // of casting char* to uint64_t*
        //
        // TODO - this only works on little endian machines .... fuuuu
        constexpr uint64_t cblock(const char *data, size_t offset = 0) noexcept
        {
            return (offset == 7) ? uint64_t(data[offset]) << (8 * offset)
                : (uint64_t(data[offset]) << (8 * offset)) | cblock(data, offset + 1);
        }

        // Mixing function for the hash function
        constexpr uint64_t cmix_h(const char *data, uint64_t h, size_t offset) noexcept
        {
            return (h ^ (crotate(cblock(data + offset) * m) * m)) * m;
        }

        // Control function for the mixing
        constexpr uint64_t cmix(const char *data, size_t len, uint64_t h, size_t offset = 0) noexcept
        {
            return (offset == (len & ~size_t(7))) ? cfinalize(data + offset, len, h)
                : cmix(data, len, cmix_h(data, h, offset), offset + 8);
        }

    } // namespace detail

    constexpr auto cexpr_murmur2_x64_64(std::string_view key, uint64_t seed) noexcept -> mm2_x64_64
    {
        uint64_t const h = detail::cmix(key.data(), key.length(), seed ^ (key.length() * detail::m));
        return mm2_x64_64{ .h = { h } };
    }

} // namespace ice::detail::murmur2_hash
