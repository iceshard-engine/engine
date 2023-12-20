/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_asyncio_win32.hxx"

namespace ice
{

    auto ice::asyncio_from_overlapped(
        OVERLAPPED* overlapped
    ) noexcept -> AsyncIOData*
    {
        return reinterpret_cast<AsyncIOData*>(overlapped);
    }

} // namespace ice