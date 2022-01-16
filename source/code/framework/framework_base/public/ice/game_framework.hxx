#pragma once
#include <ice/allocator.hxx>
#include <ice/platform_app.hxx>
#include <ice/clock.hxx>
#include <ice/uri.hxx>

namespace ice
{

    class Engine;
    class EngineRunner;

    class ResourceTracker_v2;
    class ModuleRegister;

    namespace gfx { class GfxRunner; }

    struct GameServices
    {
        virtual ~GameServices() noexcept = default;
        virtual auto resource_system() noexcept -> ice::ResourceTracker_v2 & = 0;
        virtual auto module_registry() noexcept -> ice::ModuleRegister& = 0;
    };

    class GameFramework : public ice::GameServices
    {
    public:
        GameFramework(
            ice::Allocator& alloc,
            ice::ResourceTracker_v2& resource_system,
            ice::ModuleRegister& module_register
        ) noexcept;

        virtual ~GameFramework() noexcept override;

        virtual auto config_uri() const noexcept -> ice::URI_v2 = 0;
        virtual auto graphics_world_name() const noexcept -> ice::StringID = 0;

        virtual void load_modules() noexcept = 0;

        void startup(ice::Engine& engine, ice::gfx::GfxRunner& gfx_runner) noexcept;
        void shutdown(ice::Engine& engine) noexcept;

        void game_begin(ice::EngineRunner& runner) noexcept;
        void game_end() noexcept;

        auto resource_system() noexcept -> ice::ResourceTracker_v2& final;
        auto module_registry() noexcept -> ice::ModuleRegister& final;

        auto create_app(
            ice::UniquePtr<ice::gfx::GfxRunner> gfx_runner
        ) noexcept -> ice::UniquePtr<ice::platform::App>;

    protected:
        virtual void on_app_startup_internal(ice::Engine& engine, ice::gfx::GfxRunner& gfx_runner) noexcept = 0;
        virtual void on_app_shutdown_internal(ice::Engine& engine) noexcept = 0;

        virtual void on_game_begin_internal(ice::EngineRunner& runner) noexcept = 0;
        virtual void on_game_end_internal() noexcept = 0;

    protected:
        ice::Allocator& _allocator;
        ice::SystemClock _system_clock;

    private:
        ice::ResourceTracker_v2& _resource_system;
        ice::ModuleRegister& _module_register;

        ice::Engine* _current_engine;
    };

    template<typename T>
    concept HasConfigFileMember = requires (T t) {
        { T::ConfigFile } -> std::convertible_to<ice::URI const&>;
    };
    template<typename T>
    concept HasGraphicsWorldNameMember = requires (T t) {
        { T::GraphicsWorldName } -> std::convertible_to<ice::StringID const&>;
    };
    template<typename T>
    concept HasLoadModulesMethod = requires (T t) {
        { &T::on_load_modules } -> std::convertible_to<void (T::*)(ice::GameServices&) noexcept>;
    };
    template<typename T>
    concept HasAppStartupMethod = requires (T t) {
        { &T::on_app_startup } -> std::convertible_to<void (T::*)(ice::Engine&) noexcept>;
    };
    template<typename T>
    concept HasAppShutdownMethod = requires (T t) {
        { &T::on_app_shutdown } -> std::convertible_to<void (T::*)(ice::Engine&) noexcept>;
    };
    template<typename T>
    concept HasGameStartMethod = requires (T t) {
        { &T::on_game_begin } -> std::convertible_to<void (T::*)(ice::EngineRunner&) noexcept>;
    };
    template<typename T>
    concept HasGameEndMethod = requires (T t) {
        { &T::on_game_end } -> std::convertible_to<void (T::*)(ice::EngineRunner&) noexcept>;
    };

    template<typename T>
    concept IsValidGameType = requires(T t) {
        HasLoadModulesMethod<T>;
        HasAppStartupMethod<T>;
        HasAppStartupMethod<T>;
        HasGameStartMethod<T>;
        HasGameEndMethod<T>;
    };

    template<IsValidGameType T>
    class Game : public GameFramework
    {
    public:
        Game(
            ice::Allocator& alloc,
            ice::ResourceTracker_v2& resource_tracker,
            ice::ModuleRegister& module_register
        ) noexcept
            : GameFramework{ alloc, resource_tracker, module_register }
            , _game{ alloc, _system_clock }
        { }

    private:
        auto config_uri() const noexcept -> ice::URI_v2 final
        {
            if constexpr (HasConfigFileMember<T>)
            {
                return T::ConfigFile;
            }
            else
            {
                return ice::URI_v2{ ice::scheme_file, u8"config.json" };
            }
        }

        auto graphics_world_name() const noexcept -> ice::StringID final
        {
            if constexpr (HasGraphicsWorldNameMember<T>)
            {
                return T::GraphicsWorldName;
            }
            else
            {
                return "default.graphics-world"_sid;
            }
        }

        void load_modules() noexcept
        {
            _game.on_load_modules(static_cast<GameServices&>(*this));
        }

        void on_app_startup_internal(ice::Engine& engine, ice::gfx::GfxRunner& gfx_runner) noexcept final
        {
            _game.on_app_startup(engine, gfx_runner);
        }

        void on_app_shutdown_internal(ice::Engine& engine) noexcept final
        {
            _game.on_app_shutdown(engine);
        }

        void on_game_begin_internal(ice::EngineRunner& runner) noexcept final
        {
            _game.on_game_begin(runner);
        }

        void on_game_end_internal() noexcept final
        {
            _game.on_game_end();
        }

    private:
        T _game;
    };

    auto create_game_object(
        ice::Allocator& alloc,
        ice::ResourceTracker_v2& resource_tracker,
        ice::ModuleRegister& module_register
    ) noexcept -> ice::GameFramework*;

} // namespace ice

#define ICE_REGISTER_GAMEAPP(Type) \
    auto ice::create_game_object( \
        ice::Allocator& alloc, \
        ice::ResourceTracker_v2& resource_tracker, \
        ice::ModuleRegister& module_register \
    ) noexcept -> ice::GameFramework * \
    { \
        return alloc.make<ice::Game<Type>>(alloc, resource_tracker, module_register); \
    }
