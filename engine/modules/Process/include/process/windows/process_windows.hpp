#pragma once

#include <thread>
#include <optional>

#include "native/native.h"
#include <asio.hpp>
#include <spdlog/spdlog.h>
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include <GL/glew.h>
#include <GL/wglew.h>

#include "formatter/formatter.hpp"

#include "winapi_utils.hpp"
#include "process_callbacks.hpp"

namespace astre::process::windows
{   
    /**
     * @brief WinAPI `IProcess` interface implementation
     */
    class WinapiProcess
    {
        public:
            WinapiProcess();
            WinapiProcess(const WinapiProcess &) = delete;
            WinapiProcess(WinapiProcess &&) = delete; //TODO
            WinapiProcess & operator=(const WinapiProcess &) = delete;
            WinapiProcess & operator=(WinapiProcess &&) = delete; //TODO
            ~WinapiProcess();
            //
            static LRESULT CALLBACK procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
            //
            LRESULT CALLBACK specificProcedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
        
            asio::awaitable<void> close();
            void join();

            asio::awaitable<native::window_handle> registerWindow(std::string name, unsigned int width, unsigned int height);

            asio::awaitable<bool> unregisterWindow(native::window_handle window);

            asio::awaitable<bool> setWindowCallbacks(native::window_handle window, process::WindowCallbacks && callbacks);

            asio::awaitable<native::opengl_context_handle> registerOGLContext(native::window_handle window_handle, unsigned int major_version, unsigned int minor_version);

            asio::awaitable<bool> unregisterOGLContext(native::opengl_context_handle oglctx);
            
            asio::awaitable<void> showCursor();

            asio::awaitable<void> hideCursor();

        protected:
            void messageLoop();
            bool registerClass(const WNDCLASSEX & class_structure);
            bool unregisterClass(std::string class_name);

            bool initOpenGL();

        private:
            absl::flat_hash_map<std::string, const WNDCLASSEX> _registered_classes;
            absl::flat_hash_map<native::window_handle, std::optional<WindowCallbacks>> _window_handles;

            native::opengl_context_handle _default_oglctx_handle;
            absl::flat_hash_set<native::opengl_context_handle> _oglctx_handles;

            bool _cursor_visible;

            // IO thread
            asio::io_context _io_context;
            asio::strand<asio::io_context::executor_type> _strand;
            std::thread _io_thread;
            
    };
}