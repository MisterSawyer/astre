#pragma once

#include <string_view>
#include <functional>
#include <memory>
#include <vector>
#include <algorithm>
#include <utility>

#include <asio.hpp>

#include "panel/panel_draw_context.hpp"

namespace astre::editor::panel
{
    class IPanel {
        public:

        virtual ~IPanel() = default;

        // Stable, human‑readable name used for docking/window caption.
        virtual std::string_view name() const noexcept = 0;

        // Visibility is owned by the panel to support per‑panel policy.
        virtual bool visible() const noexcept = 0;
        virtual void setVisible(bool v) noexcept = 0;

        // Called every frame to draw the panel content (ImGui, etc.).
        // Must not throw.
        virtual void draw(const DrawContext& ctx) noexcept = 0;

        // Optional async hooks for background work (icon cache, scans, etc.).
        // Default no‑ops to keep implementations minimal.
        virtual asio::awaitable<void> onStartup() { co_return; }
        virtual asio::awaitable<void> onShutdown() { co_return; }
    };
}