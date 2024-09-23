#pragma once
#include <ice/platform.hxx>
#include <ice/shard.hxx>
#include <ice/task_scheduler.hxx>

namespace ice::platform
{

    static constexpr ice::ShardID Shard_ThreadPoolSize = "platform/threads/thread-pool-size`ice::u32"_shardid;

    //! \brief Provides access to specific platform thread schedulers.
    struct Threads
    {
        virtual ~Threads() noexcept = default;

        //! \brief Returns a scheduler for the platform preferred main thread.
        virtual auto main() noexcept -> ice::TaskScheduler& = 0;

        //! \brief Returns a scheduler for the platform preferred graphics thread.
        virtual auto graphics() noexcept -> ice::TaskScheduler& = 0;

        //! \brief Returns a scheduler to a platform implementation managed thread pool.
        //! \note The number of spawned threads can be configured with the `ThreadPoolSize` shard.
        //! \warning When zero (0) threads are requestd the threadpool is not created and any task send to the scheduler will never be executed!
        virtual auto threadpool() noexcept -> ice::TaskScheduler& = 0;

        //! \brief Number of threads available in the threadpool.
        //! \note By default at least one (1), but can have more depending on platform capabilities.
        virtual auto threadpool_size() const noexcept -> ice::u32 = 0;

        //! \returns Pointer to the threadpool object managed by the platform.
        virtual auto threadpool_object() noexcept -> ice::TaskThreadPool* = 0;

        //! \returns Platform managed async-IO port to be used with various resource and network API's.
        virtual auto aio_port() const noexcept -> ice::native_aio::AIOPort { return nullptr; }
    };

    template<>
    constexpr inline ice::platform::FeatureFlags Constant_FeatureFlags<ice::platform::Threads> = FeatureFlags::Threads;

} // namespace ice::platform
