#pragma once

#include "native/native.h"
#include <asio.hpp>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_win32.h>

#include "async/async.hpp"
#include "process/process.hpp"
#include "render/render.hpp"
#include "window/window.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace astre::gui
{
    class GUIService
    {
        public:

            GUIService(process::IProcess & process, window::IWindow & window, render::IRenderer & renderer)
            :   _process(process),
                _window(window), 
                _renderer(renderer)
            {}

            asio::awaitable<void> init()
            {
                co_await _process.registerProcedureCallback(_window.getHandle(), ImGui_ImplWin32_WndProcHandler);
                
                co_await _renderer.getAsyncContext().ensureOnStrand();

                IMGUI_CHECKVERSION();
                ImGui::CreateContext();
                ImGuiIO& io = ImGui::GetIO();
                io.IniFilename = nullptr;  // Disable .ini file generation

                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

                // Setup Platform/Renderer backends
                ImGui_ImplWin32_Init(this->_window.getHandle());
                ImGui_ImplOpenGL3_Init();
            }

            asio::awaitable<void> close()
            {
                co_await _renderer.getAsyncContext().ensureOnStrand();
                     
                ImGui_ImplOpenGL3_Shutdown();
                ImGui_ImplWin32_Shutdown();
                ImGui::DestroyContext();

                co_return;
            }

            asio::awaitable<void> newFrame()
            {
                co_await _renderer.getAsyncContext().ensureOnStrand();
                
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplWin32_NewFrame();
                ImGui::NewFrame();
                
                co_return;
            }

            template<class F, class ... Args>
            asio::awaitable<void> draw(F && gui_creator, Args && ... args)
            {
                co_await _renderer.getAsyncContext().ensureOnStrand();
                
                gui_creator(std::forward<Args>(args)...);
                
                co_return;
            }

            asio::awaitable<void> render()
            {
                co_await _renderer.getAsyncContext().ensureOnStrand();
                
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                
                co_return;
            }

            render::IRenderer & getRenderer() { return _renderer; }

        private:
            process::IProcess & _process;
            render::IRenderer & _renderer;
            window::IWindow  &_window;
    };
}