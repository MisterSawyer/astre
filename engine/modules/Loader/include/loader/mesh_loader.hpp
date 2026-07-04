#pragma once

#include <string>
#include <vector>

#include "render/render.hpp"

#include "asset/concepts.hpp"

#include "proto/Render/mesh_definition.pb.h"

namespace astre::loader
{
    class MeshLoader
    {
    public:
        MeshLoader(render::IRenderer & renderer)
        : _renderer(renderer)
        {}

        // Stage 3: definition → GPU vertex buffer, keyed by name.
        asio::awaitable<bool> load(const proto::render::MeshDefinition & mesh_def) const;
        // ponytail: batch = loop over single load; no batched renderer create exists.
        asio::awaitable<bool> load(const std::vector<proto::render::MeshDefinition> & mesh_defs) const;
        asio::awaitable<bool> unload(const std::vector<std::string> & names) const;
        asio::awaitable<bool> unload(const std::string & name) const;
        asio::awaitable<bool> loadPrefabs();

    private:

        render::IRenderer & _renderer;

    };
}
