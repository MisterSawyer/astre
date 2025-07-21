#pragma once

#include "native/native.h"
#include "type/type.hpp"
#include "process_callbacks.hpp"

namespace astre::process
{

    class IProcess : public type::InterfaceBase
    {
    public:
        virtual ~IProcess() = default;
        
        /**
         * Ensures the process is properly joined and cleaned up.
         * If the I/O context is still running, spawns a coroutine to close it.
         * Waits for the I/O thread to finish execution if it is joinable.
         */
        virtual void join() = 0;
        
        /**
         * @brief Stops and closes the Windows process, unregistering all window classes and OGL contexts
         * @details This function is meant to be called when the process is no longer needed. It will stop the 
         * internal message loop, unregister all window classes and OGL contexts, and close all windows. It will
         * also stop the internal IO thread.
         * @return an awaitable that resolves when the process has been stopped and closed
         */
        virtual asio::awaitable<void> close() = 0;
        
        /**
         * @brief Register a window using the default window class.
         * @param name The name of the window to register.
         * @param resolution The resolution of the window to register.
         * @return The handle of the registered window.
         */
        virtual native::window_handle registerWindow(std::string name, unsigned int width, unsigned int height) = 0;

        /**
         * Unregister a window.
         *
         * @param window The window handle to unregister.
         *
         * @return True on success, false otherwise.
         */
        virtual asio::awaitable<bool> unregisterWindow(native::window_handle window) = 0;

        /**
         * Sets the callback functions for a specified window handle.
         *
         * This function assigns the provided callback functions to the given window handle
         * if the window is registered in the process's internal map.
         *
         * @param window The handle of the window for which callbacks are being set.
         * @param callbacks The callback functions to associate with the window.
         * @return `true` if the callbacks were successfully set, `false` if the window is not registered.
         */
        virtual asio::awaitable<bool> setWindowCallbacks(native::window_handle window, WindowCallbacks && callbacks) = 0;

        /**
         * @brief Register an OpenGL context for the given window.
         *
         * @param window_handle The window handle for which the context should be registered.
         * @param major_version The major version of the OpenGL context.
         * @param minor_version The minor version of the OpenGL context.
         *
         * @return An awaitable handle to the registered OpenGL context.
         */
        virtual asio::awaitable<native::opengl_context_handle> registerOGLContext(native::window_handle window_handle, unsigned int major_version, unsigned int minor_version) = 0;

        /**
         * Unregisters an OpenGL context from the process.
         *
         * This function removes the specified OpenGL context handle from the internal
         * set of registered contexts and deletes the context if it is found. 
         *
         * @param oglctx The OpenGL context handle to unregister.
         * @return A boolean value indicating whether the context was successfully
         *         unregistered. Returns false if the context is null, not found, or if
         *         an error occurs during deletion.
         */
        virtual asio::awaitable<bool> unregisterOGLContext(native::opengl_context_handle oglctx) = 0;

        /**
         * @brief Shows the cursor
         *
         * @note If the cursor is already shown, this method will do nothing
         */
        virtual asio::awaitable<void> showCursor() = 0;

        /**
         * @brief Hides the cursor. 
         *
         * @note If the cursor is already hidden, this function will not do anything.
         */
        virtual asio::awaitable<void> hideCursor() = 0;
    };

    template<class ProcessImplType>
    class ProcessModel final : public type::ModelBase<IProcess, ProcessImplType>
    {
        public:
            using base = type::ModelBase<IProcess, ProcessImplType>;

            template<class... Args>                                                                             
            inline ProcessModel(Args && ... args) : base(std::forward<Args>(args)...){}


            constexpr inline void join() override { return base::impl().join();}

            inline asio::awaitable<void> close() override { return base::impl().close();}
            
            inline asio::awaitable<native::window_handle> registerWindow(std::string name, unsigned int width, unsigned int height) override { 
                    return base::impl().registerWindow(std::move(name), width, height);}
            
            inline asio::awaitable<bool> unregisterWindow(native::window_handle window) override {
                    return base::impl().unregisterWindow(std::move(window));}
            
            inline asio::awaitable<bool> setWindowCallbacks(native::window_handle window, WindowCallbacks && callbacks) override {
                    return base::impl().setWindowCallbacks(std::move(window), std::move(callbacks));}

            inline asio::awaitable<native::opengl_context_handle> registerOGLContext(native::window_handle window_handle, unsigned int major_version, unsigned int minor_version) override{
                return base::impl().registerOGLContext(std::move(window_handle), major_version, minor_version);}

            inline asio::awaitable<bool> unregisterOGLContext(native::opengl_context_handle oglctx) override{
                return base::impl().unregisterOGLContext(std::move(oglctx));}

            inline asio::awaitable<void> showCursor() override{
                return base::impl().showCursor();}

            inline asio::awaitable<void> hideCursor() override{
                return base::impl().hideCursor();}

    };

    template<class ProcessImplType>
    ProcessModel(ProcessImplType && ) -> ProcessModel<ProcessImplType>;

    class Process : public type::Implementation<IProcess, ProcessModel>
    {                                                                   
        public:
            /* move ctor */
            Process(Process && other) = default;
            Process & operator=(Process && other) = default;

            /* dtor */
            virtual ~Process() = default;
        
            /* ctor */
            template<class ProcessImplType>
            Process(ProcessImplType && impl)
            : Implementation(std::move(impl))
            {}                                                          
    };
}