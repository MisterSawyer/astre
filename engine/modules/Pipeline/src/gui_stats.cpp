#include "pipeline/gui_stats.hpp"

#include "gui/gui.hpp"

namespace astre::pipeline 
{
    static void _drawDebugOverlay(float logic_fps, float logic_frame_time, const astre::render::FrameStats & stats) 
    {
        constexpr ImVec2 kWindowPos = ImVec2(10, 10);
        constexpr ImVec2 kWindowPivot = ImVec2(0.0f, 0.0f);

        ImGui::SetNextWindowPos(kWindowPos, ImGuiCond_Always, kWindowPivot);
        ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background

        ImGui::Begin("DebugOverlay", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoInputs
        );

        const ImGuiIO& io = ImGui::GetIO();

        if(io.Framerate > 0.0f){
            ImGui::Text("Render FPS: %.1f", io.Framerate);
            ImGui::Text("Render Frame Time: %.2f ms", 1000.0f / io.Framerate);
        }

        ImGui::Text("Logic FPS: %.1f", logic_fps);
        ImGui::Text("Logic Frame Time: %.2f ms", logic_frame_time);


        ImGui::Text("Draw Calls: %d", stats.draw_calls);
        ImGui::Text("Vertices: %d", stats.vertices);
        ImGui::Text("Triangles: %d", stats.triangles);

        ImGui::End();
    }

    static void _drawRenderOptionsEditor(const char* label, render::RenderOptions& options)
    {
        static const char* mode_names[] = {
            "Wireframe", "Solid", "Textured", "Shadow", "ShadowTextured"
        };

        int current_mode = static_cast<int>(options.mode);
        if (ImGui::Combo("Render Mode", &current_mode, mode_names, static_cast<int>(render::RenderMode::_COUNT)))
        {
            options.mode = static_cast<render::RenderMode>(current_mode);
        }

        bool has_offset = options.polygon_offset.has_value();
        if (ImGui::Checkbox("Enable Polygon Offset", &has_offset))
        {
            if (has_offset && !options.polygon_offset)
                options.polygon_offset = render::PolygonOffset{0.0f, 0.0f};
            else if (!has_offset)
                options.polygon_offset.reset();
        }

        if (options.polygon_offset)
        {
            ImGui::SliderFloat("Offset Factor", &options.polygon_offset->factor, -10.0f, 10.0f, "%.2f");
            ImGui::SliderFloat("Offset Units", &options.polygon_offset->units, -10.0f, 10.0f, "%.2f");
        }
    }

    bool VSYNC_ENABLED = true;
    bool VSYNC_CHANGED = true;

    static void _drawRenderControlWindow(
        render::RenderOptions& gbuffer_render_options,
        render::RenderOptions& shadow_map_render_options)
    {
        ImGui::Begin("Render Settings", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings);

        if (ImGui::CollapsingHeader("GBuffer Pass", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushID("GBuffer");
            _drawRenderOptionsEditor("GBuffer", gbuffer_render_options);
            ImGui::PopID();
        }

        if (ImGui::CollapsingHeader("Shadow Pass", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushID("Shadow");
            _drawRenderOptionsEditor("Shadow", shadow_map_render_options);
            ImGui::PopID();
        }

        ImGui::Separator();
        if(ImGui::Checkbox("Enable VSync", &VSYNC_ENABLED))
        {
            VSYNC_CHANGED = true;
        }
        ImGui::End();
    }

    asio::awaitable<void> renderGUIStats(
        gui::GUIService & gui, 
        float logic_fps, float logic_frame_time,
        const render::FrameStats & render_stats,
        render::RenderOptions & gbuffer_render_options, render::RenderOptions & shadow_map_render_options
    )
    {
        // GUI
        co_await gui.newFrame();
        co_await gui.draw(_drawDebugOverlay, logic_fps, logic_frame_time, std::cref(render_stats));
        co_await gui.draw(_drawRenderControlWindow,
            std::ref(gbuffer_render_options),
            std::ref(shadow_map_render_options));
        co_await gui.render();

        if(VSYNC_CHANGED)
        {
            VSYNC_CHANGED = false;

            if(VSYNC_ENABLED)
            {
                co_await gui.getRenderer().enableVSync();
            }
            else
            {
                co_await gui.getRenderer().disableVSync();
            }
        }
    }
}