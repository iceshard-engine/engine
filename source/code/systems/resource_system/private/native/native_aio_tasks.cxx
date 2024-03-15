/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "native_aio_tasks.hxx"

namespace ice
{

#if ISP_WINDOWS
    auto request_from_overlapped(
        OVERLAPPED* overlapped
    ) noexcept -> FileRequest*
    {
        return reinterpret_cast<FileRequest*>(overlapped);
    }
#endif

} // namespace ice
