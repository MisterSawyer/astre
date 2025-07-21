#pragma once

#include <utility>
#include <memory>
#include <array>
#include <functional>

#include <asio.hpp>

#include "native/native.h"

namespace astre::type
{
    /**
     * @brief Interface class that defines the interface for the implementation object.
     * 
     */
    class Interface
    {
        public:
        virtual ~Interface() = default;
    };

    /**
     * @brief Dispatcher class that holds the implementation object and provides access to it through the interface.
     * 
     * @tparam InterfaceType The interface type
     * @tparam ImplType The implementation type
     */
    template<class InterfaceType, class ImplType>
    class Dispatcher : public InterfaceType
    {
        public:

            // construct implementation object in place
            template<class ... Args>
            inline Dispatcher(Args && ... args)
            : _impl{std::forward<Args>(args)...}
            {}
            // construct Obj using move ctor
            explicit inline Dispatcher(ImplType && impl)
            : _impl{std::move(impl)}
            {}
            // move ctor
            inline Dispatcher(Dispatcher && other)
            :_impl{std::move(other._impl)}
            {}
            // move assign
            inline Dispatcher& operator =(Dispatcher && other)
            {
                if(this == &other)return *this;
                _impl = std::move(other._impl);
                return *this;
            }
            // no copy ctor
            Dispatcher(const Dispatcher & other) = delete;
            // no copy assign
            Dispatcher & operator = (const Dispatcher & other) = delete;
            // dtor
            virtual ~Dispatcher() = default;

            // access implementation object
            constexpr inline ImplType & impl(){return _impl;}
            // access const implementation object
            constexpr inline const ImplType & impl() const {return _impl;}

        private:
            ImplType _impl;
    };

    template<class ... Args>
    Dispatcher(Args && ... args) -> Dispatcher<Args...>;

    /**
     * @brief Simple Buffer Object
     * 
     * @tparam InterfaceType The interface type
     * @tparam buffer_size The buffer size
     * @tparam alignment The alignment
     * 
     * @details
     * This is a simple buffer object that can be used to store a single interface type.
     * If the interface type is larger than the buffer size, it will be allocated on the heap.
     */
    template<class InterfaceType, template<class> class Dispatcher, std::size_t buffer_size = 128UL, std::size_t alignment = alignof(std::max_align_t)>
    class SBO {
    public:
        using interface_type = InterfaceType;
        
        SBO() = delete;

        template<typename Impl, typename... Args>
        SBO(std::in_place_type_t<Impl>, Args&&... args)
        {
            using Model = Dispatcher<Impl>;
            if constexpr (sizeof(Model) <= buffer_size && alignof(Model) <= alignment) {
                // In-place
                new (&_buffer) Model(std::forward<Args>(args)...);
                _ptr = reinterpret_cast<InterfaceType*>(&_buffer);
                assert(_ptr != nullptr);
                _is_heap = false;
            } else {
                // Heap fallback
                _heap_obj = std::make_unique<Model>(std::forward<Args>(args)...);
                _ptr = _heap_obj.get();
                assert(_ptr != nullptr);
                _is_heap = true;
            }
        }

        // Move constructor
        SBO(SBO&& other) noexcept {
            _is_heap = other._is_heap;

            if (_is_heap) {
                // Move unique_ptr
                _heap_obj = std::move(other._heap_obj);
                _ptr = _heap_obj.get();
            } else if (other._ptr) {

                _buffer = std::move(other._buffer);
                _ptr = reinterpret_cast<InterfaceType*>(&_buffer);
            }
            // Mark other as empty
            other._ptr = nullptr;
        }

        // Move assignment
        SBO& operator=(SBO&& other) = delete;

        // copy ctor
        SBO(const SBO& other) = delete;

        // copy assign
        SBO& operator=(const SBO& other) = delete;

        // dtor
        ~SBO() 
        {
            if (_ptr == nullptr) return;
            if (!_is_heap) {
                _ptr->~InterfaceType();
            }else
            {
                _heap_obj.reset();
            }
        }

        InterfaceType* operator->() { return _ptr; }
        const InterfaceType* operator->() const { return _ptr; }
        InterfaceType& operator*() { return *_ptr; }
        const InterfaceType& operator*() const { return *_ptr; }

        explicit operator bool() const noexcept { return _ptr != nullptr; }

        bool isHeapAllocated() const noexcept { return _is_heap; }

    private:
        alignas(alignment) std::array<std::byte, buffer_size> _buffer;
        std::unique_ptr<InterfaceType> _heap_obj;
        InterfaceType* _ptr = nullptr;
        bool _is_heap = false;
    };

    /**
     * @brief Implementation class that holds a unique_ptr to the implementation object
     * and provides access to it through the interface.
     * 
     * @tparam InterfaceType The interface type
     * @tparam Dispather The dispatcher type
     */
    template<class InterfaceType, template<class> class Dispatcher>
    class Implementation final
    {
        public:
            using interface_type = InterfaceType;

            Implementation(std::nullptr_t)  = delete;

            //ctor using move ctor of ImplType
            template<std::move_constructible ImplType>
            explicit inline Implementation(ImplType && impl)
            : _pimpl(std::in_place_type<ImplType>, std::move(impl))
            {
            }

            //move ctor
            Implementation(Implementation && other)
            : _pimpl{std::move(other._pimpl)}
            {}

            //move assign
            Implementation & operator = (Implementation && other) noexcept {
                if (this != &other) {
                    _pimpl = std::move(other._pimpl);
                }
                return *this;
            }

            // copy ctor
            Implementation(const Implementation & other) = delete;
            // copy assign
            Implementation & operator = (const Implementation & other) = delete;
            //dtor
            ~Implementation() = default;

            // access object
            constexpr inline InterfaceType & operator*() noexcept {return *_pimpl; }
            // access const object
            constexpr inline const InterfaceType & operator*() const noexcept{return *_pimpl; }
            // access object pointer
            constexpr inline InterfaceType * operator->() noexcept {return _pimpl.operator->(); }
            // access const object pointer
            constexpr inline const InterfaceType * operator->() const noexcept{return _pimpl.operator->(); }
            // checks whether own implementation object
            constexpr explicit operator bool() const noexcept { return bool(_pimpl);}

        private:
            SBO<InterfaceType, Dispatcher> _pimpl;
    };
}