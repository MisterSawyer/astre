#pragma once

#include "type/type.hpp"
#include "asset/world_streamer.hpp"

namespace astre::editor::action
{
    class IAction : public type::InterfaceBase
    {
    public:
        virtual ~IAction() = default;

        virtual void apply(asset::WorldStreamer&) noexcept = 0;
        virtual void revert(asset::WorldStreamer&) noexcept = 0;
    };
}