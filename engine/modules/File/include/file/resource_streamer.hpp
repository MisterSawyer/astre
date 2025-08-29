#pragma once

#include "type/type.hpp"

namespace astre::file
{
    template<class Iterator, class ResourceData> 
    class IResourceStreamer : public type::InterfaceBase
    {
    public:
        IResourceStreamer() = default;
        virtual ~IResourceStreamer() = default;
        
        // Read data
        virtual ResourceData * read(Iterator it) = 0;
        
        // Write data
        virtual bool write(const ResourceData & data) = 0;
        
        // Remove part of the resource
        virtual bool remove(Iterator it) = 0;

        // Clear the file
        //virtual bool clear() = 0;
    };
}