/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

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
#include <ice/types.hxx>
#include <string_view>

namespace ice::detail::murmur2_hash
{

    struct mm2_x64_64
    {
        ice::u64 h[1];
    };

    constexpr auto cexpr_murmur2_x64_64(std::u8string_view key, ice::u64 seed) noexcept -> mm2_x64_64;

    constexpr auto cexpr_murmur2_x64_64(std::string_view key, ice::u64 seed) noexcept -> mm2_x64_64;

    // Murmur hash constants
    constexpr ice::u64 m = 0xc6a4a7935bd1e995;
    constexpr ice::u32 r = 47;

    constexpr auto crotate(ice::u64 a) noexcept -> ice::u64
    {
        return a ^ (a >> r);
    }

    template<typename CharType>
    constexpr auto cfinalize_h(CharType const* data, size_t key, ice::u64 h) noexcept -> ice::u64
    {
        return (key != 0) ? cfinalize_h(data, key - 1, (h ^ (ice::u64(data[key - 1]) << (8 * (key - 1))))) : h* m;
    }

    template<typename CharType>
    constexpr auto cfinalize(CharType const* data, size_t len, ice::u64 h) noexcept -> ice::u64
    {
        return (len & 7) ? crotate(crotate(cfinalize_h<CharType>(data, len & 7, h)) * m)
            : crotate(crotate(h) * m);
    }

    // reinterpret cast is illegal (static is fine) so we have to manually load 64 bit chuncks of string instead
    // of casting char* to ice::u64*
    //
    // TODO - this only works on little endian machines .... fuuuu
    template<typename CharType>
    constexpr auto cblock(CharType const* data, size_t offset = 0) noexcept -> ice::u64
    {
        return (offset == 7) ? ice::u64(data[offset]) << (8 * offset)
            : (ice::u64(data[offset]) << (8 * offset)) | cblock<CharType>(data, offset + 1);
    }

    // Mixing function for the hash function
    template<typename CharType>
    constexpr auto cmix_h(CharType const* data, ice::u64 h, size_t offset) noexcept -> ice::u64
    {
        return (h ^ (crotate(cblock<CharType>(data + offset) * m) * m)) * m;
    }

    // Control function for the mixing
    template<typename CharType>
    constexpr auto cmix(CharType const* data, size_t len, ice::u64 h, size_t offset = 0) noexcept -> ice::u64
    {
        return (offset == (len & ~size_t(7))) ? cfinalize<CharType>(data + offset, len, h)
            : cmix<CharType>(data, len, cmix_h<CharType>(data, h, offset), offset + 8);
    }

    constexpr auto cexpr_murmur2_x64_64(std::string_view key, ice::u64 seed) noexcept -> mm2_x64_64
    {
        ice::u64 const h = cmix<char>(key.data(), key.length(), seed ^ (key.length() * m));
        return mm2_x64_64{ .h = { h } };
    }

    constexpr auto cexpr_murmur2_x64_64(std::u8string_view key, ice::u64 seed) noexcept -> mm2_x64_64
    {
        ice::u64 const h = cmix<ice::utf8>(key.data(), key.length(), seed ^ (key.length() * m));
        return mm2_x64_64{ .h = { h } };
    }

} // namespace ice::detail::murmur2_hash
