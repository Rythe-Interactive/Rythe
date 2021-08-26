#pragma once

///
/// This system is not used in the project and just here for an example!!!
///

#include <core/core.hpp>

class MySystem final : public lgn::System<MySystem>
{
public:
    auto to_string(lgn::channel_format format)
    {
        switch (format)
        {
        case lgn::channel_format::eight_bit:
            return "8bit";
        case lgn::channel_format::sixteen_bit:
            return "16bit";
        case lgn::channel_format::float_hdr:
            return "hdr";
        default:
            return "unknown";
        }
    }

    auto to_string(lgn::image_components components)
    {
        switch (components)
        {
        case lgn::image_components::grey:
            return "grey";
        case lgn::image_components::grey_alpha:
            return "grey alpha";
        case lgn::image_components::rgb:
            return "rgb";
        case lgn::image_components::rgba:
            return "rgba";
        default:
            return "unknown";
        }
    }

    void setup()
    {
        using namespace legion;
        auto result = assets::load<image>(fs::view("engine://resources/default/albedo"));
        if (!result)
            log::error("{}", result.error());

        if (result.has_warnings())
            for (auto& warn : result.warnings())
                log::warn(warn);

        L_MAYBEUNUSED auto val = result.except([](L_MAYBEUNUSED exception& error) {return assets::invalid_asset<image>; });

        if(result)
        {
            auto img = result.value();
            log::debug("\nname: {}\nid: {}\nresolution: {}\nformat: {}\ncomponents: {}",
                img.name(), img.id(), img->resolution(), to_string(img->format()), to_string(img->components()));
        }

        auto objR = assets::load<mesh>(fs::view("assets://models/submeshtest.obj"));
        if (!objR)
            log::error("{}", objR.error());

        if (objR.has_warnings())
            for (auto& warn : objR.warnings())
                log::warn(warn);

        if (objR)
        {
            auto msh = objR.value();
            log::debug("\nname: {}\nid: {}\nvertex count: {}\ntriangle count: {}\nmaterial count: {}",
                msh.name(), msh.id(), msh->vertices.size(), msh->indices.size() / 3, msh->materials.size());
        }

        auto glbR = assets::load<mesh>(fs::view("assets://models/submeshtest.glb"));
        if (!glbR)
            log::error("{}", glbR.error());

        if (glbR.has_warnings())
            for (auto& warn : glbR.warnings())
                log::warn(warn);

        if (glbR)
        {
            auto msh = glbR.value();
            log::debug("\nname: {}\nid: {}\nvertex count: {}\ntriangle count: {}\nmaterial count: {}",
                msh.name(), msh.id(), msh->vertices.size(), msh->indices.size() / 3, msh->materials.size());
        }

        auto gltfR = assets::load<mesh>(fs::view("assets://models/submeshtest.gltf"));
        if (!gltfR)
            log::error("{}", gltfR.error());

        if (gltfR.has_warnings())
            for (auto& warn : gltfR.warnings())
                log::warn(warn);

        if (gltfR)
        {
            auto msh = gltfR.value();
            log::debug("\nname: {}\nid: {}\nvertex count: {}\ntriangle count: {}\nmaterial count: {}",
                msh.name(), msh.id(), msh->vertices.size(), msh->indices.size() / 3, msh->materials.size());
        }

    }

    void update(L_MAYBEUNUSED lgn::time::span deltaTime)
    {
        raiseEvent<lgn::events::exit>();
    }
};





