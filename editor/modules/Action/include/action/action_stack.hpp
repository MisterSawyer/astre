#pragma once

#include <vector>
#include <memory>

#include "file/file.hpp"

#include "action/action_interface.hpp"

namespace astre::editor::action
{
    class ActionStack 
    {
    public:
        void clear() noexcept { _done.clear(); _undone.clear(); }

        void push(std::unique_ptr<IAction> a, file::WorldStreamer& ws) noexcept
        {
            a->apply(ws);
            _undone.clear();
            _done.emplace_back(std::move(a));
        }

        bool undo(file::WorldStreamer& ws) noexcept
        {
            if (_done.empty()) return false;
            auto a = std::move(_done.back()); _done.pop_back();
            a->revert(ws);
            _undone.emplace_back(std::move(a));
            return true;
        }

        bool redo(file::WorldStreamer& ws) noexcept
        {
            if (_undone.empty()) return false;
            auto a = std::move(_undone.back()); _undone.pop_back();
            a->apply(ws);
            _done.emplace_back(std::move(a));
            return true;
        }

    private:
        std::vector<std::unique_ptr<IAction>> _done;
        std::vector<std::unique_ptr<IAction>> _undone;
    };
}