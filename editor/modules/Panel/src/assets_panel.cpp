#include <imgui.h>

#include "panel/assets_panel.hpp"

namespace astre::editor::panel
{
    // Draw a single square tile (button) + a wrapped label below.
    // Returns true if clicked. Selected tiles are highlighted.
    static bool _drawTile(const char* id, std::string_view label, float width, float height, bool selected) noexcept
    {
        ImGui::PushID(id);

        // Visual state coloring for selection
        ImGui::PushStyleColor(ImGuiCol_Button,        selected ? ImGui::GetColorU32(ImGuiCol_ButtonActive) : ImGui::GetColorU32(ImGuiCol_Button));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, selected ? ImGui::GetColorU32(ImGuiCol_ButtonActive) : ImGui::GetColorU32(ImGuiCol_ButtonHovered));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

        bool clicked = ImGui::Button("##tile", ImVec2(width, height));

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        // Center tiny “icon glyph” inside the square (single letter)
        // Pick first letter of label as a simple placeholder glyph.
        const char glyph[2] = { label.empty() ? '?' : static_cast<char>(std::toupper(label.front())), '\0' };
        ImVec2 txt = ImGui::CalcTextSize(glyph);
        ImVec2 p0  = ImGui::GetItemRectMin();
        ImVec2 p1  = ImGui::GetItemRectMax();
        ImVec2 center{ (p0.x+p1.x - txt.x)/2.0f, (p0.y+p1.y - txt.y)/2.0f };
        ImGui::GetWindowDrawList()->AddText(center, ImGui::GetColorU32(ImGuiCol_Text), glyph);

        // Label under tile (wrapped to tile width)
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + width);
        ImGui::TextWrapped("%.*s", (int)label.size(), label.data());
        ImGui::PopTextWrapPos();

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("%.*s", (int)label.size(), label.data());

        ImGui::PopID();
        return clicked;
    }


    // Grid from a set of strings (sorted for stable order).
    static void _drawGrid(const absl::flat_hash_set<std::string>& set,
                         std::string& selection,
                         float tile_size, float padding) noexcept
    {
        std::vector<std::string> items;
        items.reserve(set.size());
        for (const auto& s : set) items.emplace_back(s);
        std::sort(items.begin(), items.end());

        const ImGuiStyle& style = ImGui::GetStyle();
        const float cell_w = tile_size * 2.0f;
        const float cell_h_min = tile_size + ImGui::GetTextLineHeightWithSpacing(); // at least 1 line label height
        const float spacing = style.ItemSpacing.x + padding;

        // Compute columns from avail width
        const float avail_w = std::max(0.0f, ImGui::GetContentRegionAvail().x);
        const int columns = std::max(1, (int)std::floor((avail_w + spacing) / (cell_w + spacing)));

        int col = 0;
        for (const auto& it : items)
        {
            if (col > 0) ImGui::SameLine(0.0f, spacing);

            ImGui::BeginGroup(); // lock tile + label as one cell
            bool clicked = _drawTile(it.c_str(), it, cell_w, tile_size, selection == it);
            ImGui::Dummy(ImVec2(0.0f, std::max(0.0f, cell_h_min - tile_size - ImGui::GetItemRectSize().y))); // pad to minimum height
            ImGui::EndGroup();

            if (clicked)
                selection = it;

            // advance col
            col = (col + 1) % columns;
            if (col == 0) ImGui::NewLine();
        }

        if (items.empty())
        {
            ImGui::TextDisabled("No items in this folder.");
        }
    }

    // Left pane: small helpers that also switch the current view when clicked.
    static void _folderBullet(const char* label, bool selected) noexcept
    {
        if (selected) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_Text));
        ImGui::BulletText("%s", label);
        if (selected) ImGui::PopStyleColor();
    }


    void AssetsPanel::draw(const model::DrawContext& ctx) noexcept
    {
        if (!_visible) return;

        // dock into the node
        if (ctx.assets_dock_id != 0)
            ImGui::SetNextWindowDockID(ctx.assets_dock_id, ImGuiCond_Always);

        // No close button => cannot be closed by the user
        ImGuiWindowFlags flags =    
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoTitleBar;

        if (ImGui::Begin("Assets", &_visible, flags)) {
            ImGui::TextUnformatted("Asset Browser");
            ImGui::Separator();

            // === Left: folders =================================================
            ImGui::BeginChild("##folders", ImVec2(220, 0), true);

            if (ImGui::TreeNodeEx("assets", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // Clickable ‘folders’ that switch the right-hand content
                bool models_selected = (_current_view == View::Models);
                if (ImGui::Selectable("models/", models_selected))
                    _current_view = View::Models;

                //ImGui::BulletText("textures/");  // placeholder
                //ImGui::BulletText("materials/");
                //ImGui::BulletText("scenes/");

                ImGui::TreePop();
            }

            if (ImGui::TreeNode("resources"))
            {
                bool shaders_selected = (_current_view == View::Shaders);
                if (ImGui::Selectable("shaders/", shaders_selected))
                    _current_view = View::Shaders;

                bool scripts_selected = (_current_view == View::Scripts);
                if (ImGui::Selectable("scripts/", scripts_selected))
                    _current_view = View::Scripts;

                //ImGui::BulletText("fonts/");

                ImGui::TreePop();
            }

            ImGui::EndChild();

            ImGui::SameLine();

            // === Right: content ===============================================
            ImGui::BeginChild("##content", ImVec2(0, 0), true);

            // Header bar (path + view controls)
            const char* path_label = "";
            switch (_current_view)
            {
                case View::Models:  path_label = "Folder: assets/models"; break;
                case View::Shaders: path_label = "Folder: resources/shaders"; break;
                case View::Scripts: path_label = "Folder: resources/scripts"; break;
                default:            path_label = "Folder: -"; break;
            }

            ImGui::TextDisabled("%s", path_label);

            ImGui::Separator();

            // Tiles
            switch (_current_view)
            {
                case View::Models:
                    //_drawGrid(_tracker.getLoadedVertexBuffers(), _selected_item, _tile_size, _tile_padding);
                    break;
                case View::Shaders:
                    //_drawGrid(_tracker.getLoadedShaders(), _selected_item, _tile_size, _tile_padding);
                    break;
                case View::Scripts:
                    //_drawGrid(_tracker.getLoadedScripts(), _selected_item, _tile_size, _tile_padding);
                    break;
                default:
                    ImGui::TextDisabled("Select a folder on the left.");
                    break;
            }

            ImGui::EndChild();

            (void)ctx;
        }
        ImGui::End();
    }
}