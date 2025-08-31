#pragma once

#include "type/type.hpp"

namespace astre::file
{
    template<class Key, class ResourceData> 
    class IResourceStreamer : public type::InterfaceBase
    {
    public:
        IResourceStreamer() = default;
        virtual ~IResourceStreamer() = default;
        
        // Read data
        virtual ResourceData * read(Key key) = 0;
        
        // Write data
        virtual bool write(const ResourceData & data) = 0;
        
        // Remove part of the resource
        virtual bool remove(Key key) = 0;

        // Clear the file
        //virtual bool clear() = 0;
    };
}