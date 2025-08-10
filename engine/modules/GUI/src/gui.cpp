#include "gui/gui.hpp"

namespace astre::gui
{
    extern "C" LRESULT CALLBACK Astre_ImGuiProxyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        // Just enqueue; DO NOT call any ImGui API on this thread.
        astre::gui::GUIService::enqueueMsg(msg, wParam, lParam);
        return 0; // Let your process layer decide whether to DefWindowProc afterwards if needed.
    }
}