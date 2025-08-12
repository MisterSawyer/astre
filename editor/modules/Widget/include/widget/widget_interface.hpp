#pragma once

namespace astre::editor::widget
{
    class IWidget
    {
        public:
            virtual ~IWidget() = default;

            virtual bool draw() = 0;
    };
}