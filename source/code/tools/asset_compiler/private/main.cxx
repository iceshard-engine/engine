#include <core/memory.hxx>
#include <core/platform/utility.hxx>
#include <core/platform/windows.hxx>
#include <resource/modules/filesystem_module.hxx>
#include <resource/modules/dynlib_module.hxx>
#include <asset_system/asset_system.hxx>
#include <asset_system/asset_module.hxx>

#include <CLI/CLI.hpp>

int main(int argc, char** argv)
{
    if constexpr (core::build::is_release == false)
    {
        core::memory::globals::init_with_stats();
    }
    else
    {
        core::memory::globals::init();
    }

    // The main allocator object
    auto& main_allocator = core::memory::globals::default_allocator();

    // Special proxy allocators for the game and internal systems.
    core::memory::proxy_allocator filesystem_allocator{ "resource-filesystem", main_allocator };
    core::memory::proxy_allocator dynlib_allocator{ "resource-dynlib", main_allocator };

    resource::ResourceSystem resource_system{ main_allocator };

    {
        auto working_dir = core::working_directory(main_allocator);
        fmt::print("Initializing filesystem module at: {}\n", working_dir);

        core::pod::Array<core::stringid_type> schemes{ core::memory::globals::default_scratch_allocator() };
        core::pod::array::push_back(schemes, resource::scheme_file);
        core::pod::array::push_back(schemes, resource::scheme_directory);
        resource_system.add_module(
            core::memory::make_unique<resource::ResourceModule, resource::FileSystem>(
                main_allocator, filesystem_allocator, working_dir
            ),
            schemes
        );

        core::pod::array::clear(schemes);
        core::pod::array::push_back(schemes, resource::scheme_dynlib);
        resource_system.add_module(
            core::memory::make_unique<resource::ResourceModule, resource::DynLibSystem>(main_allocator, dynlib_allocator),
            schemes
        );
    }

    // CLI data
    CLI::App app{ };

    std::string asset_name;
    app.add_option("-a,--asset", asset_name, "The asset to compile.")
        ->required();

    core::Map<std::string, asset::AssetType> types{ main_allocator };
    types.emplace("tex", asset::AssetType::Texture);
    types.emplace("texture", asset::AssetType::Texture);
    types.emplace("mesh", asset::AssetType::Mesh);

    asset::AssetType asset_type;
    app.add_option("-t,--type", asset_type, "The asset type.")
        ->transform(CLI::Transformer(types))
        ->required();

    std::string input_file;
    app.add_option("-i,--input", input_file, "The input file to be preprocessed.")
        ->required();

    std::string output_file;
    app.add_option("-o,--output", output_file, "The resulting output file.")
        ->required();

    std::vector<std::string> modules_path_list;
    app.add_option("--modules", modules_path_list, "Location of Ice modules.")
        ->default_val(".");

    //std::vector<std::string> serializers;
    //code_emitter.add_option("-s,--serializer", serializers, "Serializers to be used, will default to all if none is chosen.");

    //std::string parser;
    //code_emitter.add_option("-p,--parser", parser, "The parser to be used if multiple are available, fails if none is chosen");

    CLI11_PARSE(app, argc, argv);

    // Initial mount points
    {
        using resource::URI;
        using resource::URN;

        resource_system.mount(URI{ resource::scheme_dynlib, "bin" });

        asset::AssetSystem asset_system{ main_allocator, resource_system };
        core::Vector<core::memory::unique_pointer<iceshard::AssetModule>> loaded_modules{ main_allocator };
        for (auto const& module_name : modules_path_list)
        {
            auto* const assimp_module_location = resource_system.find(URN{ "asset_module.dll" });
            if (assimp_module_location != nullptr)
            {
                loaded_modules.emplace_back(
                    iceshard::load_asset_module(
                        main_allocator,
                        assimp_module_location->location().path,
                        asset_system
                    )
                );
            }
        }

        resource_system.flush_messages();
        resource_system.mount(URI{ resource::scheme_directory, "../source/data" });

        asset_system.update();

        asset::AssetData data;
        asset::AssetStatus status = asset_system.read(
            asset::Asset{ asset_name.c_str(), asset_type },
            data
        );

        if (status != asset::AssetStatus::Compiled)
        {
            if (status != asset::AssetStatus::Available)
            {
                fmt::print(stderr, "Failed to compile asset {}", asset_name);
            }
            else
            {
                fmt::print(stderr, "Trying to re-compile asset {}, skiping...", asset_name);
            }
        }
        else
        {
            fmt::print("Asset compiled {}", asset_name);

            core::data_view header_magic;
            header_magic._data = "ISRA";
            header_magic._size = 4;

            core::Buffer header_meta{ main_allocator };
            resource::store_meta_view(data.metadata, header_meta);

            uint32_t header_size = 4 + 4 + core::buffer::size(header_meta);
            core::data_view header_size_data;
            header_size_data._data = &header_size;
            header_size_data._size = 4;

            resource::OutputResource* asset_out = resource_system.open(URI{ resource::scheme_file, "out/test.icr" });
            asset_out->write(header_magic);
            asset_out->write(header_size_data);
            asset_out->write(header_meta);
            asset_out->write(data.content);
            asset_out->flush();
        }
    }

    return 0;
}
