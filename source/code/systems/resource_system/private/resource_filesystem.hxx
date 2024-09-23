/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/native_aio.hxx>
#include <ice/resource.hxx>
#include <ice/task_expected.hxx>

namespace ice
{

    class FileSystemResource : public ice::Resource
    {
    public:
        virtual auto size() const noexcept -> ice::usize = 0;

        virtual auto load_data(
            ice::Allocator& alloc,
            ice::Memory& memory,
            ice::String fragment,
            ice::native_aio::AIOPort aioport
        ) const noexcept -> ice::TaskExpected<ice::Data> = 0;

        ice::u32 data_index;
    };

} // namespace ice
