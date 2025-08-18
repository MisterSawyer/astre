#pragma once

#include "type/type.hpp"
#include "file/file.hpp"

namespace astre::editor::action
{
    class IAction : public type::InterfaceBase
    {
    public:
        virtual ~IAction() = default;

        virtual void apply(file::WorldStreamer&) noexcept = 0;
        virtual void revert(file::WorldStreamer&) noexcept = 0;
    };
}