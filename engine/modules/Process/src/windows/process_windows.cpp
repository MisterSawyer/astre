#include <sstream>
#include <string>

#include <spdlog/spdlog.h>
#include <GL/glew.h>
#include <GL/wglew.h>

#include "formatter/formatter.hpp"
#include "process/process.hpp"
#include "process/windows/process_windows.hpp"
#include "process/windows/winapi_utils.hpp"

namespace astre::process
{
    Process createProcess(unsigned int number_of_threads)
    {
        return Process(std::in_place_type<windows::WinapiProcess>, number_of_threads);
    }
}

namespace astre::process::windows
{
    // -------------------------------------------
    // ProcedureContext
    // -------------------------------------------
    ProcedureContext::ProcedureContext()
    {
        start(&ProcedureContext::messageLoop, this);
    }

    void ProcedureContext::messageLoop() 
    { 
        spdlog::debug("[winapi-procedure] Message loop started");

        MSG msgcontainer;

        while(running())
        {     
            // perform implementation part of messages from the queue
            // first call to this function will create message queue for this thread in winapi
            if(PeekMessage(&msgcontainer, NULL, 0, 0, PM_REMOVE))
            {
                switch(msgcontainer.message)
                {
                    case WM_QUIT: 
                    {
                        spdlog::debug("[winapi-procedure] Input thread received WM_QUIT");

                        // handle rest of messages
                        while(PeekMessage(&msgcontainer, 0, 0, 0, PM_REMOVE) != 0)
                        {
                            TranslateMessage(&msgcontainer);
                            //will call specific procedure of give n WM_ message in current thread
                            DispatchMessage(&msgcontainer);
                            // handle other tasks
                            poll();
                        }
                        // handle rest of tasks
                        run();
                        // all msg handled
                        close();

                        break;
                    }

                    default:
                    {
                        TranslateMessage(&msgcontainer);
                        //will call specific procedure of give n WM_ message in current thread
                        DispatchMessage(&msgcontainer);
                        break;
                    }
                }
            }

            // handle other tasks
            poll();
        }

        spdlog::debug(std::format("[winapi-procedure] Input thread ended"));
    }

    // -------------------------------------------
    // WinapiProcess
    // -------------------------------------------
    WinapiProcess::WinapiProcess(unsigned int number_of_threads)
    :   _default_oglctx_handle(nullptr),
        _cursor_visible(true),
        _procedure_context(),
        _execution_context(number_of_threads)
    {
        spdlog::debug("[winapi] WinapiProcess constructor called");
    }

    WinapiProcess::~WinapiProcess()
    {
        spdlog::debug("[winapi]WinapiProcess destructor called");

        join();

        assert(_window_handles.empty() == true);
        assert(_registered_classes.empty() == true);
        assert(_oglctx_handles.empty() == true);
        assert(_default_oglctx_handle == nullptr);
    }
    

    const IProcess::execution_context_type & WinapiProcess::getExecutionContext() const
    {
        return _execution_context;
    }

    IProcess::execution_context_type & WinapiProcess::getExecutionContext()
    {
        return _execution_context;
    }

    void WinapiProcess::join()
    {
        // wait for all consumers tasks to finish
        _execution_context.join();
        
        spdlog::debug("[winapi]WinApi process joining");

        // request thread to close
        _procedure_context.co_spawn(close());

        // join IO thread
        _procedure_context.join();
        
        spdlog::debug("[winapi] WinApi process thread joined");
    }

    asio::awaitable<void> WinapiProcess::close()
    {
        spdlog::debug("[winapi] WinapiProcess clearing data");

        while(_oglctx_handles.empty() == false){
            co_await unregisterOGLContext(*(_oglctx_handles.begin()));
        }
        _default_oglctx_handle = nullptr;

        for(const auto & [window_handle, callbacks] : _window_handles)
        {
            PostMessage(window_handle, WM_CLOSE, 0, 0);
        }

        while(_registered_classes.empty() == false){
            unregisterClass(_registered_classes.begin()->first);
        }

        spdlog::debug("[winapi]  WinapiProcess stopping and closing");

        PostQuitMessage(0);
        _procedure_context.close();
    }

    bool WinapiProcess::registerClass( const WNDCLASSEX & class_structure)
    {
        std::string name = (std::stringstream{} << class_structure.lpszClassName).str();

        spdlog::debug(std::format("[winapi-class-manager] registering WinApi class {} ...", name));

        if(name.empty()){
            spdlog::error(std::format("[winapi-class-manager] Name of a class cannot be empty"));
            return false;
        }

        if(_registered_classes.contains(name)){
            spdlog::error(std::format("[winapi-class-manager] class {} already registered", name));
            return false;
        }

        _registered_classes.try_emplace(name, std::move(class_structure));

        if(RegisterClassEx(&_registered_classes.at(name)) == false){
            spdlog::error("winapi-class-manager] RegisterClassEx failed");
            return false;
        }
        
        spdlog::info(std::format("[winapi-class-manager] WinApi class {} created", name));

        return true;
    }

    bool WinapiProcess::unregisterClass(std::string name)
    {
        spdlog::debug(std::format("[winapi-class-manager] unregistering WinApi class {} ...", name));

        if(name.empty()){
            spdlog::error(std::format("[winapi-class-manager] Name of a class cannot be empty"));
            return false;
        }

        if(_registered_classes.contains(name) == false){
            spdlog::error(std::format("[winapi-class-manager] class {} not registered", name));
            return false;
        }

        UnregisterClass(name.c_str(), GetModuleHandle(nullptr));
        _registered_classes.erase(name);

        spdlog::info(std::format("[winapi-class-manager] WinApi class {} destroyed", name));

        return true;
    }

    asio::awaitable<native::window_handle> WinapiProcess::registerWindow(std::string name, unsigned int width, unsigned int height)
    {
        co_await _procedure_context.ensureOnStrand();

        spdlog::debug("[winapi] registerWindow");

        const std::string class_name = "WINAPI:window";

        if(_registered_classes.contains(class_name) == false){
            spdlog::debug(std::format("[winapi] Window class not registered. Trying to register now ..."));

            const bool registration_result = registerClass(
                    defaultWindowClass<WinapiProcess *>(WinapiProcess::procedure, class_name)
            );

            if(registration_result == false){
                spdlog::error(std::format("[winapi] Cannot register Window class"));
                
                co_return nullptr;
            }
        }

        const auto class_structure = _registered_classes.at(class_name);

        spdlog::debug(std::format("[winapi] Process DPI setting awareness to DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2"));
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        native::window_handle window_handle = CreateWindowExA(
            0,                                  // dwExStyle
            class_name.c_str(),                  // lpClassName
            name.c_str(),                 // lpWindowName
            WS_OVERLAPPEDWINDOW,                // dwStyle
            CW_USEDEFAULT,                      // X
            CW_USEDEFAULT,                      // Y
            (int)width,              // nWidth
            (int)height,             // nHeight
            NULL,                               //hWndParent
            NULL,                               // hMenu
            class_structure.hInstance,   // hInstance
            nullptr                             // lpParam
        );

        if(window_handle != nullptr){
            spdlog::debug(std::format("[winapi] constructing window [{}] ", window_handle));
        }else{
            spdlog::error(std::format("[winapi] constructing window failed"));
            
            co_return nullptr;
        }

        // store pointer to instance of process in winapi class
        SetWindowLongPtrW(window_handle, 0, reinterpret_cast<LONG_PTR>(this));

        native::device_context device_context = GetDC(window_handle);
        if(device_context == nullptr){
            spdlog::error(std::format("[winapi] cannot retrive device context for {}", window_handle));
            DestroyWindow(window_handle);
            co_return nullptr;
        }

        PIXELFORMATDESCRIPTOR pfd = generateAdvancedPFD();
        const int pixel_format_ID = ChoosePixelFormat(device_context, &pfd);
        spdlog::debug(std::format("[winapi] choosen pixel format ID {} ", pixel_format_ID));

        if(pixel_format_ID <= 0 || SetPixelFormat(device_context, pixel_format_ID, &pfd) == false) {
            spdlog::error(std::format( "[winapi] cannot set pixel format for {}", window_handle));
            ReleaseDC(window_handle, device_context);
            DestroyWindow(window_handle);
            co_return nullptr;
        }

        ReleaseDC(window_handle, device_context);

        _window_handles.try_emplace(window_handle, std::nullopt);

        spdlog::info(std::format("[winapi] Window {} created", window_handle));

        RAWINPUTDEVICE rid;
        rid.usUsagePage = 0x01; // Generic desktop controls
        rid.usUsage = 0x02;     // Mouse
        rid.dwFlags = 0; // Receive input only when focused
        rid.hwndTarget = window_handle;

        if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
            spdlog::error("Failed to register raw input device: {}", GetLastError());
            DestroyWindow(window_handle);  // clean up
            co_return nullptr;
        }

        co_return window_handle;
    }

    asio::awaitable<bool> WinapiProcess::unregisterWindow(native::window_handle window)
    {
        co_await _procedure_context.ensureOnStrand();

        spdlog::debug(std::format("[winapi] unregistering Window {} ...", window));

        if(_window_handles.contains(window) == false){
            spdlog::error(std::format("[winapi-class-manager] window {} not registered", window));
            co_return false;
        }

        ReleaseDC(window, GetDC(window));
        DestroyWindow(window);
        
        _window_handles.erase(window);

        spdlog::info(std::format("[winapi-class-manager] window {} destroyed", window));
        co_return true;
    }

    asio::awaitable<bool> WinapiProcess::setWindowCallbacks(native::window_handle window, WindowCallbacks && callbacks)
    {
        co_await _procedure_context.ensureOnStrand();

        spdlog::debug("[winapi] setWindowCallbacks");

        if(_window_handles.contains(window) == false){
            spdlog::error(std::format("[winapi-class-manager] window {} not registered", window));
            co_return false;
        }

        _window_handles.at(window) = std::move(callbacks);
        co_return true;
    }

    bool WinapiProcess::initOpenGL()
    {
        if(_default_oglctx_handle != nullptr){
            spdlog::debug(std::format("[winapi] default context already created"));
            return true;
        }
        spdlog::debug(std::format("[winapi] creating default context"));

        const std::string class_name = "WINAPI:oglContextWindow";
        if(_registered_classes.contains(class_name) == false){
            spdlog::debug(std::format("[winapi] Window class not registered. Trying to register now ..."));

            const bool registration_result = registerClass(
                    defaultWindowClass<WinapiProcess *>(WinapiProcess::procedure, class_name)
            );

            if(registration_result == false){
                spdlog::error(std::format("[winapi] Cannot register Window class"));
                return false;
            }
        }

        const auto & class_structure = _registered_classes.at(class_name);
        
        native::window_handle ogl_window = CreateWindowExA(
            0,                                  // dwExStyle
            class_name.c_str(),                  // lpClassName
            "oglContextWindow",            // lpWindowName
            WS_OVERLAPPEDWINDOW,                // dwStyle
            CW_USEDEFAULT,                      // X
            CW_USEDEFAULT,                      // Y
            1,              // nWidth
            1,             // nHeight
            NULL,                               //hWndParent
            NULL,                               // hMenu
            class_structure.hInstance,   // hInstance
            nullptr                             // lpParam
        );

        HDC device_handle = GetDC(ogl_window);
        PIXELFORMATDESCRIPTOR pfd = generateAdvancedPFD();
        int pixel_format_ID = ChoosePixelFormat(device_handle, &pfd);

        if(pixel_format_ID <= 0 || SetPixelFormat(device_handle, pixel_format_ID, &pfd) == false) {
            spdlog::error(std::format("[winapi-procedure] Cannot set pixel format to device {}" , GetLastError()));
            spdlog::debug("[winapi-procedure] removing ogl context window handle...");
            ReleaseDC(ogl_window, device_handle);
            DestroyWindow(ogl_window);
            return false;
        }

        _default_oglctx_handle = wglCreateContext(device_handle);

        if(_default_oglctx_handle == nullptr){
            spdlog::error(std::format("[winapi-procedure] Cannot create default opengl context {}",  GetLastError()));
            ReleaseDC(ogl_window, device_handle);
            DestroyWindow(ogl_window);
            return false;
        }

        wglMakeCurrent(device_handle, _default_oglctx_handle);

        //init glew
        glewExperimental = true;
        if(glewInit() != GLEW_OK){
            spdlog::error("[winapi-procedure] Cannot initialize GLEW");
        }else{
            spdlog::info("[winapi-procedure] GLEW initialized properly");
        }
        
        spdlog::info(std::format("\nOpenGL\n  vendor: {}\n  renderer: {}\n  version: {}\n  shading language version: {}", 
            glGetString(GL_VENDOR), 
            glGetString(GL_RENDERER), 
            glGetString(GL_VERSION),
            glGetString(GL_SHADING_LANGUAGE_VERSION)));
        
        wglMakeCurrent(0, 0);
        ReleaseDC(ogl_window, device_handle);
        DestroyWindow(ogl_window);

        spdlog::info(std::format("[winapi-procedure] Default opengl {} created", _default_oglctx_handle));

        _oglctx_handles.emplace(_default_oglctx_handle);

        return true;
    }

    asio::awaitable<native::opengl_context_handle> WinapiProcess::registerOGLContext(native::window_handle window_handle, unsigned int major_version, unsigned int minor_version)
    {
        co_await _procedure_context.ensureOnStrand();

        if(window_handle == nullptr){
            spdlog::error(std::format("[winapi-procedure] cannot construct OGLContext on empty window "));
            co_return nullptr;
        }

        const auto window_it = _window_handles.find(window_handle);
        if(window_it == _window_handles.end())
        {
            spdlog::error(std::format("[winapi-procedure] cannot find window {} for constructing OGLContext ", window_handle));
            co_return nullptr;
        }

        auto device_handle = GetDC(window_handle);
        if(device_handle == nullptr){
            spdlog::error(std::format("[winapi-procedure] Cannot obtain device handle for window {}, err: {}", window_handle,  GetLastError()));
            co_return nullptr;
        }

        if(_default_oglctx_handle == nullptr)
        {
            if(initOpenGL() == false || _default_oglctx_handle == nullptr)
            {
                spdlog::error(std::format("[winapi] cannot obtain default context, err: {}", GetLastError()));
                co_return nullptr;
            }
        }

        static const int CONTEXT_ATTRIBUTES[] =
        {
            WGL_CONTEXT_MAJOR_VERSION_ARB, (const int)major_version,
            WGL_CONTEXT_MINOR_VERSION_ARB, (const int)minor_version,
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
            0
        };

        native::opengl_context_handle oglctx  = wglCreateContextAttribsARB(device_handle, _default_oglctx_handle, CONTEXT_ATTRIBUTES);
        wglMakeCurrent(device_handle, oglctx);
        wglMakeCurrent(0, 0);
        ReleaseDC(window_handle, device_handle);

        if(oglctx == nullptr){
            spdlog::error(std::format("[winapi-procedure] Cannot create opengl context {}", GetLastError()));
            co_return nullptr;
        }

        _oglctx_handles.emplace(oglctx);
        
        spdlog::info(std::format("[winapi-procedure] created opengl context {}", oglctx));

        co_return oglctx;
    }

    asio::awaitable<bool> WinapiProcess::unregisterOGLContext(native::opengl_context_handle oglctx)
    {
        co_await _procedure_context.ensureOnStrand();
        
        spdlog::debug(std::format("[winapi] unregistering OGLContext {} ...", oglctx));

        if(oglctx == nullptr)
        {
            spdlog::error(std::format("[winapi-procedure] cannot unregister OGLContext on empty context"));
            co_return false;
        }

        const auto oglctx_it = _oglctx_handles.find(oglctx);
        if(oglctx_it == _oglctx_handles.end())
        {
            spdlog::error(std::format("[winapi-procedure] cannot find OGLContext {} for unregistering", oglctx));
            co_return false;
        }

        if(wglDeleteContext(oglctx) == false)
        {
            spdlog::error(std::format("[winapi-procedure] cannot delete OGLContext {}, err: {}", oglctx, GetLastError()));
            co_return false;
        }

        _oglctx_handles.erase(oglctx);

        spdlog::info(std::format("[winapi-procedure] OGLContext {} destroyed", oglctx));

        co_return true;
    }

    asio::awaitable<void> WinapiProcess::showCursor()
    {
        co_await _procedure_context.ensureOnStrand();

        _cursor_visible = true;
        ShowCursor(TRUE);
    }

    asio::awaitable<void> WinapiProcess::hideCursor()
    {
        co_await _procedure_context.ensureOnStrand();

        _cursor_visible = false;
        ShowCursor(FALSE);
    }

    LRESULT CALLBACK WinapiProcess::procedure(native::window_handle window, UINT message, WPARAM wparam, LPARAM lparam)
    {
        WinapiProcess * specific_process = reinterpret_cast<WinapiProcess *>(GetWindowLongPtrW(window, 0));
        // call specific procedure
        if ( specific_process != nullptr ) return specific_process->specificProcedure(window, message, wparam, lparam);
        // call default procedure
        return DefWindowProc(window, message, wparam, lparam);
    }

    LRESULT CALLBACK WinapiProcess::specificProcedure(native::window_handle window, UINT message, WPARAM wparam, LPARAM lparam)
    {
        if(window == nullptr)
        {
            spdlog::error(std::format("[winapi-procedure] window is null"));
            return DefWindowProc(window, message, wparam, lparam);
        }

        if(_window_handles.contains(window) == false)
        {
            spdlog::error(std::format("[winapi-procedure] window {} not registered", window));
            return DefWindowProc(window, message, wparam, lparam);
        }

        auto & window_callbacks_result = _window_handles.at(window);
        if(window_callbacks_result.has_value() == false)
        {
            return DefWindowProc(window, message, wparam, lparam);
        }
        auto & window_callbacks = window_callbacks_result.value();

        switch (message)
        {
        case WM_CREATE:
        {
            spdlog::debug(std::format("[winapi-procedure] WM_CREATE received for window {}", window));
            return DefWindowProc(window, message, wparam, lparam);
        }

        //WM_CLOSE is sent to the window when it is being closed - when its "X" button is clicked, 
        //or "Close" is chosen from the window's menu,
        // or Alt-F4 is pressed while the window has focus, etc.
        case WM_CLOSE:
        {
            spdlog::debug(std::format("[winapi-procedure] WM_CLOSE received, for window {}", window));
            if(window_callbacks.onDestroy != nullptr)
            {
                // spawn onDestroy callback on window_callbacks executor
                // it will handle the window destruction
                window_callbacks.context.co_spawn(window_callbacks.onDestroy());
            }
            else
            {
                // if no callback is set, destroy window synchronously
                _window_handles.at(window) = std::nullopt;
                return DefWindowProc(window, message, wparam, lparam); 
            } 

            return 0;
        }

        case WM_PAINT :
        {
            ValidateRect(window, nullptr);
            return 0;
        }

        case WM_SIZE:
        {
            spdlog::debug(std::format("[winapi-procedure] WM_SIZE received, for window {}", window));
            auto handling_result = DefWindowProc(window, message, wparam, lparam);
            if(window_callbacks.onResize != nullptr)
            {
                window_callbacks.context.co_spawn(window_callbacks.onResize((int)LOWORD(lparam), (int)HIWORD(lparam)));
            }
            return handling_result;
        }

        case WM_KEYDOWN: 
        {
            int key = (int)wparam;

            auto handling_result = DefWindowProc(window, message, wparam, lparam);
            if(window_callbacks.onKeyPress != nullptr)
            {
                window_callbacks.context.co_spawn(window_callbacks.onKeyPress(key));
            }
            return handling_result;
        }

        case WM_KEYUP : 
        {
            int key = (int)wparam;

            auto handling_result = DefWindowProc(window, message, wparam, lparam);
            if(window_callbacks.onKeyRelease != nullptr)
            {
                window_callbacks.context.co_spawn(window_callbacks.onKeyRelease(key));
            }
            return handling_result;
        }

        case WM_INPUT:
        {
            UINT size = 0;
            GetRawInputData((HRAWINPUT)lparam, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
            if (size == 0) break;

            std::vector<BYTE> buffer(size);
            if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) != size)
                break;

            RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer.data());
            if (raw->header.dwType == RIM_TYPEMOUSE) {
                LONG dx = raw->data.mouse.lLastX;
                LONG dy = raw->data.mouse.lLastY;

                POINT screen_pos;
                GetCursorPos(&screen_pos);

                POINT client_pos = screen_pos;
                ScreenToClient(window, &client_pos);

                if(window_callbacks.onMouseMove != nullptr){
                    window_callbacks.context.co_spawn(
                        window_callbacks.onMouseMove(
                            (float)client_pos.x, (float)client_pos.y,
                            (float)dx, (float)dy
                        )
                    );
                }
            }

            if(_cursor_visible == false)
            {
                // cursor locked
                RECT rect;
                if (GetClientRect(window, &rect)) {
                    POINT center = {
                        (rect.right - rect.left) / 2,
                        (rect.bottom - rect.top) / 2
                    };
                    ClientToScreen(window, &center);
                    SetCursorPos(center.x, center.y);
                }
            }

            break;
        }

        case WM_LBUTTONDOWN:
        {
            if (window_callbacks.onMouseButtonDown)
            {
                window_callbacks.context.co_spawn(window_callbacks.onMouseButtonDown(MK_LBUTTON));
            }
            return 0;
        }

        case WM_LBUTTONUP:
        {
            if (window_callbacks.onMouseButtonUp)
            {
                window_callbacks.context.co_spawn(window_callbacks.onMouseButtonUp(MK_LBUTTON));
            }
            return 0;
        }

        default:
            return DefWindowProc(window, message, wparam, lparam);
        }
    }
}