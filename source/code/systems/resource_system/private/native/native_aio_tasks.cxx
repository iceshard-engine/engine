/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "native_aio_tasks.hxx"

namespace ice
{

#if ISP_WINDOWS
    auto asyncio_from_overlapped(
        OVERLAPPED* overlapped
    ) noexcept -> AsyncIOData*
    {
        return reinterpret_cast<AsyncIOData*>(overlapped);
    }
#endif

} // namespace ice
