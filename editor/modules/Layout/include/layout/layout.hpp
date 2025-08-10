#pragma once

namespace astre::editor::layout
{
    class DockSpace
    {
    public:
        DockSpace() = default;
        ~DockSpace() = default;

        void build();

        unsigned int getLeftDockId() const { return _left_dock_id; }

        unsigned int getRightDockId() const { return _right_dock_id; }

        unsigned int getBottomDockId() const { return _bottom_dock_id; }

        unsigned int getCenterDockId() const { return _center_dock_id; }

    private:
        ImGuiID _dockspace_id = 0;

        ImGuiID _center_dock_id = 0;
        ImGuiID _left_dock_id = 0;
        ImGuiID _right_dock_id = 0;
        ImGuiID _bottom_dock_id = 0;
    };
}