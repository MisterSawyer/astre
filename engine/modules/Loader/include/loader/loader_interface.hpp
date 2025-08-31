#pragma once

#include "type/type.hpp"

namespace astre::loader
{
    class ILoader : public type::InterfaceBase
    {
    public:
        virtual ~ILoader() = default;
    };
}