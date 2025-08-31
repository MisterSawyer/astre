#pragma once

#include "loader/shader_loader.hpp"
#include "loader/mesh_loader.hpp"
#include "loader/entity_loader.hpp"
#include "loader/entity_serializer.hpp"
#include "loader/script_loader.hpp"

#include "file/resource_streamer.hpp"

namespace astre::loader
{
    template<class Key, class ResourceData, class Loader> 
    asio::awaitable<bool> loadStreamedResources(std::vector<Key> resources, const file::IResourceStreamer<Key, ResourceData> & streamer, Loader & target)
    {
        for(const auto & resource : resources)
        {
            ResourceData * data = streamer.read(resource);
            if(!data) co_return false;

            co_await target.load(*data);
        }

        co_return true;
    }
}