#pragma once

#include "native/native.h"
#include "type.hpp"
#include "process/process.hpp"

namespace astre::window
{
    class IWindow : public type::InterfaceBase
    {
    public:
        virtual ~IWindow() = default;

        /**
         * @brief Check if the window is still valid.
         * @return If the window is still valid and usable, @c true. Otherwise, @c false.
         */
        virtual bool good() const = 0;

        /**
         * @brief Asynchronously shows the window.
         * 
         * If the current thread is not the one associated with the strand, 
         * the operation is dispatched to the correct strand to maintain thread safety.
         */
        virtual asio::awaitable<void> show() = 0;
        
        /**
         * @brief Asynchronously hides the window
         * 
         * When the returned awaitable is awaited, the window is hidden.
         * The underlying window handle is not modified.
         * 
         * This function is thread-safe and can be called from any thread
         * since it always dispatches the work to the window's strand.
         */
        virtual asio::awaitable<void> hide() = 0;

        /**
         * Closes the window by unregistering it from the associated process.
         *
         * This function ensures that the window is closed on the correct strand. 
         * If the window handle is valid, it will unregister the window with the 
         * associated process and set the window handle to nullptr.
         *
         * @return An awaitable that resolves when the window has been successfully 
         *         unregistered and closed.
         */
        virtual asio::awaitable<void> close() = 0;

        virtual unsigned int getWidth() const = 0;

        virtual unsigned int getHeight() const = 0;

        /**
         * @brief Retrieves the native window handle.
         * 
         * @return The handle to the native window associated with this window instance.
         */
        virtual native::window_handle getHandle() const = 0;

        /**
         * Acquires a device context for the current window.
         *
         * This function retrieves a device context (DC) handle for the window
         * associated with this instance. It logs the acquisition
         * of the device context and returns the acquired handle.
         *
         * @return The acquired device context handle.
         */
        virtual native::device_context acquireDeviceContext() = 0;

        /**
         * Releases the specified device context from the window.
         *
         * @param device_context The device context to release.
         * @return True if the device context was successfully released; false otherwise.
         *
         * The function logs an error and returns false if the provided device context is null,
         * if the device context was not previously acquired, or if it does not match the
         * currently acquired device context. It also logs an error if releasing the device context fails.
         * Upon successful release, the acquired device context is set to nullopt and a success message is logged.
         */
        virtual bool releaseDeviceContext(native::device_context device_context) = 0;

        /**
         * Returns a reference to the process associated with this window.
         *
         * @return A reference to the IProcess object representing the process that owns this window.
         */
        virtual process::IProcess & getProcess() = 0;
    };

    template<class WindowImplType>
    class WindowModel final : public type::ModelBase<IWindow, WindowImplType>
    {
        public:
            using base = type::ModelBase<IWindow, WindowImplType>;

            template<class... Args>                                                                             
            inline WindowModel(Args && ... args) : base(std::forward<Args>(args)...){} 

            constexpr inline bool good() const override { return base::impl().good();}
            inline asio::awaitable<void> show() override {return base::impl().show();}
            inline asio::awaitable<void> hide() override {return base::impl().hide();}
            inline asio::awaitable<void> close() override {return base::impl().close();}
            constexpr inline native::window_handle getHandle() const override { return base::impl().getHandle();}

            constexpr inline native::device_context acquireDeviceContext() override { 
                return base::impl().acquireDeviceContext();};
            constexpr inline bool releaseDeviceContext(native::device_context device_context) override { 
                return base::impl().releaseDeviceContext(std::move(device_context));};

            constexpr inline const Resolution & getResolution() const override {return base::impl().getResolution();}
            constexpr inline IProcess & getProcess() override {return base::impl().getProcess();}
    
            
            inline void move(type::InterfaceBase * dest) override
            {
                ::new(dest) WindowModel(std::move(base::impl()));
            }

            inline void copy([[maybe_unused]] type::InterfaceBase * dest) const override
            {
                throw std::runtime_error("Not copyable");
            }

            inline std::unique_ptr<type::InterfaceBase> clone() const override
            {
                throw std::runtime_error("Not copyable");
            }
    };

    template<class WindowImplType>
    WindowModel(WindowImplType && ) -> WindowModel<WindowImplType>;

    class Window : public type::Implementation<IWindow, WindowModel>
    {                                                                   
        public:
            /* move ctor */
            Window(Window && other) = default;
            Window & operator=(Window && other) = default;

            /* dtor */
            virtual ~Window() {}
        
            /* ctor */
            template<class WindowImplType>
            Window(WindowImplType && impl)
            : Implementation(std::move(impl))
            {}                                                          
    };
}