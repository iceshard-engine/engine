/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT
#include "resource_asyncio_win32.hxx"

#if ISP_WINDOWS

namespace ice
{

    auto ice::asyncio_from_overlapped(
        OVERLAPPED* overlapped
    ) noexcept -> AsyncIOData*
    {
        return reinterpret_cast<AsyncIOData*>(overlapped);
    }

} // namespace ice

#endif // #if ISP_WINDOWS
