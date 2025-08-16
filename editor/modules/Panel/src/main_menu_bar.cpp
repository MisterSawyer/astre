#include <imgui.h>

#include "panel/main_menu_bar.hpp"

namespace astre::editor::panel
{
    void MainMenuBar::draw(const model::DrawContext& ctx) noexcept 
    {
        if (!_visible) return;

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                // TODO: wire to ProjectModel / actions
                ImGui::MenuItem("New Project...", nullptr, false, false);
                ImGui::MenuItem("Open Project...", nullptr, false, false);
                ImGui::MenuItem("Save", "Ctrl+S", false, false);
                ImGui::MenuItem("Save As...", "Ctrl+Shift+S", false, false);
                ImGui::Separator();
                ImGui::MenuItem("Exit", nullptr, false, false);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit")) {
                // TODO: wire to CommandStack
                ImGui::MenuItem("Undo", "Ctrl+Z", false, false);
                ImGui::MenuItem("Redo", "Ctrl+Y", false, false);
                ImGui::Separator();
                ImGui::MenuItem("Preferences...", nullptr, false, false);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Window")) {
                // In a follow‑up, you can expose callbacks to toggle panel visibilities.
                ImGui::MenuItem("Scene", nullptr, true, false);
                ImGui::MenuItem("Properties", nullptr, true, false);
                ImGui::MenuItem("Assets", nullptr, true, false);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help")) {
                ImGui::MenuItem("About Astre Editor", nullptr, false, false);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
        (void)ctx;
    }
}
