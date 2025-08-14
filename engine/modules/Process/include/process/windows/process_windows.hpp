#pragma once

#include <optional>

#include "native/native.h"
#include <asio.hpp>
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include "async/async.hpp"

#include "process_callbacks.hpp"

namespace astre::process::windows
{ 
    
    class ProcedureContext : public async::ThreadContext
    {
        public:
            ProcedureContext();

            ~ProcedureContext() = default;

            void messageLoop();
    };

    /**
     * @brief WinAPI `IProcess` interface implementation
     */
    class WinapiProcess
    {
        public:
            WinapiProcess(unsigned int number_of_threads);

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

            asio::awaitable<void> registerProcedureCallback(native::window_handle window, native::procedure callback);

            asio::awaitable<native::opengl_context_handle> registerOGLContext(native::window_handle window_handle, unsigned int major_version, unsigned int minor_version);

            asio::awaitable<bool> unregisterOGLContext(native::opengl_context_handle oglctx);
            
            asio::awaitable<void> showCursor();

            asio::awaitable<void> hideCursor();

            const IProcess::execution_context_type & getExecutionContext() const;
            IProcess::execution_context_type & getExecutionContext();


        protected:
            bool _initOpenGL();

            bool _registerClass(const WNDCLASSEX & class_structure);
            bool _unregisterClass(std::string class_name);

            bool _registerRawInputDevices();
            bool _unregisterRawInputDevices();

        private:
            absl::flat_hash_map<std::string, const WNDCLASSEX> _registered_classes;
            absl::flat_hash_map<native::window_handle, std::optional<WindowCallbacks>> _window_handles;

            bool _rid_registered;

            native::opengl_context_handle _default_oglctx_handle;
            absl::flat_hash_set<native::opengl_context_handle> _oglctx_handles;

            bool _cursor_visible;

            // IO thread
            // run in the same thread as procedure and specificProcedure
            ProcedureContext _procedure_context;

            // Consumers execution context
            // runs in thread pool - can run on different threads
            IProcess::execution_context_type _execution_context;

            //hooked procedure callbacks, to forward events
            absl::flat_hash_map<native::window_handle, native::procedure> _window_procedures;
    };
}