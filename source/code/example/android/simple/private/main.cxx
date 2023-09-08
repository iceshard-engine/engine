#include <jni.h>
#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/sensor.h>
#include <android/log.h>

#include <android/native_activity.h>

#include <ice/string/string.hxx>
#include <ice/task.hxx>
#include <ice/task_queue.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/task_thread.hxx>
#include <ice/task_thread_pool.hxx>
#include <ice/task_utils.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/mem_allocator_host.hxx>

#include <ice/log_module.hxx>
#include <ice/module_register.hxx>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <ice/app.hxx>

struct ice::app::Config { ice::Allocator& alloc; };
struct ice::app::State {
    State(ice::Allocator& alloc) noexcept
        : alloc{ alloc }
        , mreg{ ice::create_default_module_register(alloc) }
    {
        mreg->load_module(alloc, ice::load_log_module, ice::unload_log_module);
        ice::TaskThreadPoolCreateInfo const tpinfo {
            .thread_count = 4
        };
        tpool = ice::create_thread_pool(alloc, global_queue, tpinfo);
    }
    ice::Allocator& alloc;
    ice::UniquePtr<ice::ModuleRegister> mreg;
    ice::TaskQueue global_queue;
    ice::UniquePtr<ice::TaskThreadPool> tpool;
};
struct ice::app::Runtime { ice::Allocator& alloc; };

template<typename T>
void destroy_object(T* obj) noexcept
{
    obj->alloc.destroy(obj);
}

void ice_init(
    ice::Allocator& alloc,
    ice::app::Factories& factories
) noexcept
{
    using namespace ice::app;

    factories.factory_config = [](ice::Allocator& alloc) -> ice::UniquePtr<ice::app::Config> { return ice::make_unique<Config>(&destroy_object<Config>, alloc.create<Config>(alloc)); };
    factories.factory_state = [](ice::Allocator& alloc) -> ice::UniquePtr<ice::app::State> { return ice::make_unique<State>(&destroy_object<State>, alloc.create<State>(alloc)); };
    factories.factory_runtime = [](ice::Allocator& alloc) -> ice::UniquePtr<ice::app::Runtime> { return ice::make_unique<Runtime>(&destroy_object<Runtime>, alloc.create<Runtime>(alloc)); };
}

auto ice_setup(
    ice::Allocator& alloc,
    ice::app::Arguments const& args,
    ice::app::Config& config,
    ice::app::State& state
) noexcept -> ice::Result
{
    return ice::app::S_ApplicationResume;
}

auto test(ice::TaskScheduler sched) noexcept -> ice::Task<>
{
    ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Game, "Test {}", syscall(__NR_gettid));
    co_await sched;
    ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Game, "Test {}", syscall(__NR_gettid));
    co_return;
};

extern "C"
{

JNIEXPORT jstring JNICALL Java_net_example_simple_MyApp_stringFromJNI(
    JNIEnv* env,
    jobject thiz
)
{
    static ice::HostAllocator alloc;
    static ice::TaskQueue queue;
    auto mreg = ice::create_default_module_register(alloc);
    mreg->load_module(alloc, ice::load_log_module, ice::unload_log_module);

    ice::TaskScheduler sched{ queue };
    auto thread = ice::create_thread(alloc, queue, { .exclusive_queue = true });
    ice::ManualResetEvent ev;
    ice::manual_wait_for(test(sched), ev);
    ICE_LOG(ice::LogSeverity::Retail, ice::LogTag::Game, "Test");
    ev.wait();
    thread = nullptr;

    return (*env).NewStringUTF("Hello from JNI! Compiled with ABI arm64.");
}

}
