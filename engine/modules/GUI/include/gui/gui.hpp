#pragma once

#include <atomic>
#include <array>

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

namespace astre::gui::detail
{
    //TODO move to specyfic impl
    struct Win32Msg { UINT msg; WPARAM wParam; LPARAM lParam; };

    // Single Producer Single Consumer Queue
    template<std::size_t N>
    class SPSQQueue 
    {
        static_assert((N & (N - 1)) == 0, "N must be power of two");

    public:
        bool push(const Win32Msg& v) noexcept 
        {
            auto h = _head.load(std::memory_order_relaxed);
            auto nh = (h + 1) & (N - 1);
            if (nh == _tail.load(std::memory_order_acquire)) return false; // full
            _buf[h] = v;
            _head.store(nh, std::memory_order_release);
            return true;
        }

        bool pop(Win32Msg& out) noexcept 
        {
            auto t = _tail.load(std::memory_order_relaxed);
            if (t == _head.load(std::memory_order_acquire)) return false; // empty
            out = _buf[t];
            _tail.store((t + 1) & (N - 1), std::memory_order_release);
            return true;
        }

    private:
        std::atomic<uint32_t> _head{0}, _tail{0};
        std::array<Win32Msg, N> _buf{};
    };
}

namespace astre::gui
{
    // Forward-declared proxy proc (C linkage/ no captures)
    extern "C" LRESULT CALLBACK Astre_ImGuiProxyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
                // will run independently in the process IO thread
                co_await _process.registerProcedureCallback(_window.getHandle(), Astre_ImGuiProxyWndProc);
                
                co_await _renderer.getAsyncContext().ensureOnStrand();

                IMGUI_CHECKVERSION();
                ImGui::CreateContext();
                ImGuiIO& io = ImGui::GetIO();
                io.IniFilename = nullptr;  // Disable .ini file generation

                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
                io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

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

                // Drain queued Win32 messages on the render strand BEFORE NewFrame
                detail::Win32Msg m;
                while (_msgQueue.pop(m)) {
                    ImGui_ImplWin32_WndProcHandler(_window.getHandle(), m.msg, m.wParam, m.lParam);
                }

                ImGui::NewFrame();
                
                co_return;
            }

            template<class F, class ... Args>
            asio::awaitable<void> draw(F && gui_creator, Args && ... args)
            {
                co_await _renderer.getAsyncContext().ensureOnStrand();
                
                std::invoke(std::forward<F>(gui_creator), std::forward<Args>(args)...);
                
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

            // Called by the proxy WndProc on the IO/message thread:
            static bool enqueueMsg(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
                return _msgQueue.push({msg, wParam, lParam});
            }

        private:
            process::IProcess & _process;
            render::IRenderer & _renderer;
            window::IWindow  &_window;

            // Single global SPSC queue used by the proxy and drained by the render strand.
            static inline detail::SPSQQueue<2048> _msgQueue{};
    };
}