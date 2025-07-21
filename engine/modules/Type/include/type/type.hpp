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
    class InterfaceBase
    {
        public:
        virtual ~InterfaceBase() = default;

        virtual void move(InterfaceBase * dest) = 0;
        virtual void copy(InterfaceBase * dest) const = 0;
        virtual std::unique_ptr<InterfaceBase> clone() const = 0;
    };

    template<template<class> class ModelTemplate>
    struct ModelTraits;

    /**
     * @brief ModelBase class that holds the implementation object and provides access to it through the interface.
     * 
     * @tparam InterfaceType The interface type
     * @tparam ImplType The implementation type
     */
    template<class InterfaceType, class ImplType>
    class ModelBase : public InterfaceType
    {
        public:
            using interface_t = InterfaceType;

            // construct impl object in place
            template<class ... Args>
            inline ModelBase(Args && ... args)
            : _impl{std::forward<Args>(args)...}
            {}

            // construct impl using move ctor
            explicit inline ModelBase(ImplType && impl)
            : _impl{std::move(impl)}
            {}

            // construct impl using copy ctor
            explicit inline ModelBase(const ImplType & impl)
            : _impl{impl}
            {}

            // move ctor
            inline ModelBase(ModelBase && other)
            :_impl{std::move(other._impl)}
            {}

            // copy ctor
            ModelBase(const ModelBase & other)
            : _impl{other._impl}
            {}

            // move assign
            inline ModelBase& operator =(ModelBase && other)
            {
                if(this == &other)return *this;
                _impl = std::move(other._impl);
                return *this;
            }

            // copy assign
            ModelBase & operator = (const ModelBase & other)
            {
                if(this == &other)return *this;
                _impl = other._impl;
                return *this;
            }

            // dtor
            virtual ~ModelBase() = default;

            // access implementation object
            constexpr inline ImplType & impl(){return _impl;}
            // access const implementation object
            constexpr inline const ImplType & impl() const {return _impl;}

        private:
            ImplType _impl;
    };

    template<class ... Args>
    ModelBase(Args && ... args) -> ModelBase<Args...>;

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
    template<template<class> class ModelTemplate, std::size_t buffer_size = 128UL, std::size_t alignment = alignof(std::max_align_t)>
    class SBO {
    public:
        static_assert(std::is_class_v<typename ModelTraits<ModelTemplate>::interface_t>, 
              "ModelTraits must define interface_t for given ModelTemplate");
        using interface_t = ModelTraits<ModelTemplate>::interface_t;
        
        SBO() = delete;

        template<typename Impl, typename... Args>
        SBO(std::in_place_type_t<Impl>, Args&&... args)
        {
            using Model = ModelTemplate<Impl>;
            if constexpr (sizeof(Model) <= buffer_size && alignof(Model) <= alignment) {
                // In-place
                ::new (&_buffer) Model(std::forward<Args>(args)...);
                _ptr = reinterpret_cast<interface_t*>(&_buffer);
                assert(_ptr != nullptr);
                _is_heap = false;
            } else {
                // Heap fallback
                _heap_obj = std::make_unique<Model>(std::forward<Args>(args)...);
                _ptr = static_cast<interface_t*>(_heap_obj.get());
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
                _ptr = static_cast<interface_t*>(_heap_obj.get());
            } else if (other._ptr) {
                // In-place
                other._ptr->move(reinterpret_cast<InterfaceBase*>(&_buffer));
                _ptr = reinterpret_cast<interface_t*>(&_buffer);
                assert(_ptr != nullptr);
            }
            // Mark other as empty
            other._ptr = nullptr;
        }

        // copy constructor
        SBO(const SBO& other) noexcept {
            _is_heap = other._is_heap;

            if (_is_heap) {
                // Copy content of unique_ptr
                _heap_obj = other._ptr->clone();
                _ptr = static_cast<interface_t*>(_heap_obj.get());
            } else if (other._ptr) {
                // In-place
                other._ptr->copy(reinterpret_cast<InterfaceBase*>(&_buffer));
                _ptr = reinterpret_cast<interface_t*>(&_buffer);
                assert(_ptr != nullptr);
            }
        }

        // Move assignment
        SBO& operator=(SBO&& other) = delete;

        // copy assign
        SBO& operator=(const SBO& other) = delete;

        // dtor
        ~SBO() 
        {
            if (_ptr == nullptr) return;
            if (!_is_heap) {
                _ptr->~interface_t();
            }else
            {
                _heap_obj.reset();
            }
        }

        interface_t* operator->() { return _ptr; }
        const interface_t* operator->() const { return _ptr; }
        interface_t& operator*() { return *_ptr; }
        const interface_t& operator*() const { return *_ptr; }

        explicit operator bool() const noexcept { return _ptr != nullptr; }

        bool isHeapAllocated() const noexcept { return _is_heap; }

    private:
        alignas(alignment) std::array<std::byte, buffer_size> _buffer;
        std::unique_ptr<InterfaceBase> _heap_obj;
        interface_t* _ptr = nullptr;
        bool _is_heap = false;
    };

    /**
     * @brief Implementation class that holds a unique_ptr to the implementation object
     * and provides access to it through the interface.
     * 
     * @tparam ModelTemplate
     */
    template<template<class> class ModelTemplate>
    class Implementation final
    {
        public:
            using interface_t = ModelTraits<ModelTemplate>::interface_t;

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
            constexpr inline interface_t & operator*() noexcept {return *_pimpl; }
            // access const object
            constexpr inline const interface_t & operator*() const noexcept{return *_pimpl; }
            // access object pointer
            constexpr inline interface_t * operator->() noexcept {return _pimpl.operator->(); }
            // access const object pointer
            constexpr inline const interface_t * operator->() const noexcept{return _pimpl.operator->(); }
            // checks whether own implementation object
            constexpr explicit operator bool() const noexcept { return bool(_pimpl);}

        private:
            SBO<ModelTemplate> _pimpl;
    };
}