#include <imgui.h>
#include <imgui_internal.h>

#include <spdlog/spdlog.h>

#include "ecs/ecs.hpp"

#include "panel/properties_panel.hpp"

namespace astre::editor::panel 
{
    
    static bool _drawTransformComponent(ecs::TransformComponent & transform)
    {
        bool changed = false;

        float position[3] = {transform.position().x(), transform.position().y(), transform.position().z()};
        float rotation[4] = {transform.rotation().w(), transform.rotation().x(), transform.rotation().y(), transform.rotation().z()}; // {0.f, 0.f, 0.f};
        float scale[3]    = {transform.scale().x(), transform.scale().y(), transform.scale().z()};

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) 
        {
            ImGui::TextUnformatted("Position");
            ImGui::PushItemWidth(-FLT_MIN); // full width
            changed |= ImGui::DragFloat3("##pos", position, 0.1f);
            ImGui::PopItemWidth();

            ImGui::TextUnformatted("Rotation");
            ImGui::PushItemWidth(-FLT_MIN); // full width
            changed |= ImGui::DragFloat4("##rot", rotation, 0.1f);
            ImGui::PopItemWidth();

            ImGui::TextUnformatted("Scale");
            ImGui::PushItemWidth(-FLT_MIN); // full width
            changed |= ImGui::DragFloat3("##scale", scale, 0.1f);
            ImGui::PopItemWidth();

            // apply changes
            if(changed)
            {
                transform.mutable_position()->set_x(position[0]);
                transform.mutable_position()->set_y(position[1]);
                transform.mutable_position()->set_z(position[2]);

                transform.mutable_rotation()->set_w(rotation[0]);
                transform.mutable_rotation()->set_x(rotation[1]);
                transform.mutable_rotation()->set_y(rotation[2]);
                transform.mutable_rotation()->set_z(rotation[3]);

                transform.mutable_scale()->set_x(scale[0]);
                transform.mutable_scale()->set_y(scale[1]);
                transform.mutable_scale()->set_z(scale[2]);
            }
        }

        return changed;
    }

    static bool _drawVisualComponent(ecs::VisualComponent& visual) noexcept
    {
        bool changed = false;

        if (ImGui::CollapsingHeader("Visual", ImGuiTreeNodeFlags_DefaultOpen)) 
        {
            // Visibility toggle
            {
                bool vis = visual.visible();
                ImGui::PushItemWidth(-FLT_MIN); // full width
                if (ImGui::Checkbox("Visible", &vis)) {
                    visual.set_visible(vis);
                    changed = true;
                }
                ImGui::PopItemWidth();
            }

            // Vertex buffer name
            {
                std::string vbuf = visual.vertex_buffer_name();
                char buf[256];
                std::strncpy(buf, vbuf.c_str(), sizeof(buf));
                buf[sizeof(buf) - 1] = '\0';
                ImGui::TextUnformatted("Vertex Buffer");
                ImGui::PushItemWidth(-FLT_MIN); // full width
                if (ImGui::InputText("##vertex_buffer", buf, sizeof(buf))) {
                    visual.set_vertex_buffer_name(buf);
                    changed = true;
                }
                ImGui::PopItemWidth();
            }

            // Shader name
            {
                std::string shd = visual.shader_name();
                char buf[256];
                std::strncpy(buf, shd.c_str(), sizeof(buf));
                buf[sizeof(buf) - 1] = '\0';
                ImGui::TextUnformatted("Shader");
                ImGui::PushItemWidth(-FLT_MIN); // full width
                if (ImGui::InputText("##shader", buf, sizeof(buf))) {
                    visual.set_shader_name(buf);
                    changed = true;
                }
                ImGui::PopItemWidth();
            }

            // Color
            {
                float colArr[4] = { visual.color().x(), visual.color().y(), visual.color().z(), visual.color().w() };

                ImGui::TextUnformatted("Color");
                ImGui::PushItemWidth(-FLT_MIN); // full width
                if (ImGui::ColorEdit4("##visual_color", colArr, ImGuiColorEditFlags_PickerHueBar)) {
                    auto* c = visual.mutable_color();
                    c->set_x(colArr[0]);
                    c->set_y(colArr[1]);
                    c->set_z(colArr[2]);
                    c->set_w(colArr[3]);
                    changed = true;
                }
                ImGui::PopItemWidth();
            }
        }

        return changed;
    }


    static bool _drawLightComponent(ecs::LightComponent & light) noexcept
    {
        bool changed = false;

        if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) 
        {
            // --- Light Type ---
            {
                int type_idx = static_cast<int>(light.type());
                const char* items[] = { "Directional", "Point", "Spot" };
                ImGui::TextUnformatted("Light Type");
                ImGui::PushItemWidth(-FLT_MIN); // full width
                if (ImGui::Combo("##light_type", &type_idx, items, IM_ARRAYSIZE(items))) {
                    light.set_type(static_cast<astre::ecs::LightType>(type_idx + 1));
                    changed = true;
                }
                ImGui::PopItemWidth();
            }
            // Cast shadows
            {
                bool cs = light.cast_shadows();
                ImGui::PushItemWidth(-FLT_MIN); // full width
                if (ImGui::Checkbox("Cast Shadows", &cs)) {
                    light.set_cast_shadows(cs);
                    changed = true;
                }
                ImGui::PopItemWidth();
            }
            
            ImGui::SeparatorText("Color & Intensity");
            {
                float colArr[3] = { light.color().x(), light.color().y(), light.color().z()};
                ImGui::TextUnformatted("Color");
                ImGui::PushItemWidth(-FLT_MIN); // full width
                if (ImGui::ColorEdit3("##light_color", colArr, ImGuiColorEditFlags_PickerHueBar)) {
                    auto* c = light.mutable_color();
                    c->set_x(colArr[0]);
                    c->set_y(colArr[1]);
                    c->set_z(colArr[2]);
                    changed = true;
                }
                ImGui::PopItemWidth();

                float intensity = light.color().w();
                ImGui::TextUnformatted("Intensity");
                ImGui::PushItemWidth(-FLT_MIN); // full width
                if(ImGui::DragFloat("##intensity", &intensity, 0.1f, 0.0f, 100.0f, "%.3f", ImGuiSliderFlags_None))
                {
                    auto* c = light.mutable_color();
                    c->set_w(intensity);
                    changed = true;
                }
                ImGui::PopItemWidth();

            // Attenuation (Point/Spot only)
            const bool show_atten = (light.type() == astre::ecs::LightType::POINT ||
                                     light.type() == astre::ecs::LightType::SPOT);
            if (show_atten)             
            {
                ImGui::SeparatorText("Attenuation");

                float constant = light.has_constant()  ? light.constant()  : 1.0f;
                float linear = light.has_linear()    ? light.linear()    : 0.0f;
                float quadratic = light.has_quadratic() ? light.quadratic() : 0.0f;

                ImGui::TextUnformatted("Constant");
                ImGui::PushItemWidth(-FLT_MIN); // full width
                if(ImGui::DragFloat("##constant", &constant, 0.1f, 0.0f, 1.0f, "%.3f"))
                {
                    light.set_constant(constant);
                    changed = true;
                }
                ImGui::PopItemWidth();

                ImGui::TextUnformatted("Linear");
                ImGui::PushItemWidth(-FLT_MIN); // full width
                if(ImGui::DragFloat("##linear", &linear, 0.1f, 0.0f, 1.0f, "%.3f"))
                {
                    light.set_linear(linear);
                    changed = true;
                }
                ImGui::PopItemWidth();

                ImGui::TextUnformatted("Quadratic");
                ImGui::PushItemWidth(-FLT_MIN); // full width
                if(ImGui::DragFloat("##quadratic", &quadratic, 0.1f, 0.0f, 1.0f, "%.3f"))
                {
                    light.set_quadratic(quadratic);
                    changed = true;
                }
                ImGui::PopItemWidth();
            }

            // Spot-only controls (inner/outer cutoffs in DEGREES UI, stored as COSINE)
            const bool show_spot = (light.type() == astre::ecs::LightType::SPOT);
            if (show_spot)
            {
                ImGui::SeparatorText("Spot Angles");

                float inner_deg = light.has_inner_cutoff() ? math::degreesFromCos(light.inner_cutoff()) : 15.0f;
                float outer_deg = light.has_outer_cutoff() ? math::degreesFromCos(light.outer_cutoff()) : 30.0f;

                // Inner
                ImGui::TextUnformatted("Inner Angle");
                ImGui::PushItemWidth(-FLT_MIN); // full width
                bool ch_inner = ImGui::SliderFloat("##inner_angle", &inner_deg, 0.0f, 90.0f, "%.1f°");
                ImGui::PopItemWidth();
                // Outer
                ImGui::TextUnformatted("Outer Angle");
                ImGui::PushItemWidth(-FLT_MIN); // full width
                bool ch_outer = ImGui::SliderFloat("##outer_angle", &outer_deg, 0.0f, 90.0f, "%.1f°");
                ImGui::PopItemWidth();

                // Maintain relation: inner <= outer
                if (inner_deg > outer_deg) {
                    // If user just changed one, bind the other
                    if (ch_inner && !ch_outer) outer_deg = inner_deg;
                    else if (!ch_inner && ch_outer) inner_deg = outer_deg;
                    else /*both*/ inner_deg = outer_deg;
                }

                const float inner_cos = math::cosFromDegrees(inner_deg);
                if (!light.has_inner_cutoff() || std::fabs(inner_cos - light.inner_cutoff()) > 1e-6f) {
                    light.set_inner_cutoff(inner_cos);
                    changed = true;
                }

                const float outer_cos = math::cosFromDegrees(outer_deg);
                if (!light.has_outer_cutoff() || std::fabs(outer_cos - light.outer_cutoff()) > 1e-6f) {
                    light.set_outer_cutoff(outer_cos);
                    changed = true;
                }

                // (Optional) show read-only cosine values for debugging
                ImGui::TextDisabled("cos(inner)=%.4f\ncos(outer)=%.4f",
                    light.has_inner_cutoff() ? light.inner_cutoff() : math::cosFromDegrees(15.0f),
                    light.has_outer_cutoff() ? light.outer_cutoff() : math::cosFromDegrees(30.0f));
            }

            } // color & intensity
        }
        
        return changed;
    }

    void PropertiesPanel::setSelectedEntityDef(std::optional<std::pair<world::ChunkID, ecs::EntityDefinition>> def)
    { 
        _selected_entity_def = std::move(def);
        if(_selected_entity_def)spdlog::debug("[editor] Selected entity def: {}", _selected_entity_def->second.name());
    }

    void PropertiesPanel::draw(const DrawContext& ctx) noexcept 
    {
        if (!_visible) return;

        // dock into the node
        if (ctx.properties_dock_id != 0)
            ImGui::SetNextWindowDockID(ctx.properties_dock_id, ImGuiCond_Always);

        // No close button => cannot be closed by the user
        ImGuiWindowFlags flags =    
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoTitleBar;


        if (ImGui::Begin("Properties", &_visible, flags)) {
            ImGui::TextUnformatted("Inspector");
            ImGui::Separator();

            if(_selected_entity_def)
            {

                auto & [chunk_id, entity] = *_selected_entity_def;
                ImGui::TextUnformatted(entity.name().c_str());
                ImGui::Separator();

                if(entity.has_transform())
                {
                    _properties_changed |= _drawTransformComponent(*(entity.mutable_transform()));
                    ImGui::Separator();
                }

                if(entity.has_visual())
                {
                    _properties_changed |= _drawVisualComponent(*(entity.mutable_visual()));
                    ImGui::Separator();
                }

                if(entity.has_light())
                {
                    _properties_changed |= _drawLightComponent(*(entity.mutable_light()));
                    ImGui::Separator();
                }

            }
        }
        ImGui::End();
    }
}