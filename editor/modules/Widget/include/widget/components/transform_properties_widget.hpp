#pragma once

#include "ecs/ecs.hpp"

#include "widget/widget_interface.hpp"

namespace astre::editor::widget
{
    class TransformPropertiesWidget : public IWidget
    {
    public:
        TransformPropertiesWidget() = default;
        ~TransformPropertiesWidget() = default;

        bool draw() override {return false;};

    private:

    };
}