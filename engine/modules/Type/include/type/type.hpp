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
    };

    template<typename T>
    concept HasMove = requires(T obj, T* dst) {
        { obj.move(dst) } -> std::same_as<void>;
    };

    template<typename T>
    concept HasCloneAndCopy = requires(const T& obj, T* dst) {
        // clone() must return something convertible to unique_ptr<T>
        { obj.clone() } -> std::convertible_to<std::unique_ptr<T>>;
    
        // copy(T*) must be callable (usually returns void)
        { obj.copy(dst) } -> std::same_as<void>;
    };

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
    template<class InterfaceType, template<class> class ModelTemplate, std::size_t buffer_size = 128UL, std::size_t alignment = alignof(std::max_align_t)>
    class SBO {
    public:
        using interface_t = InterfaceType;
        
        // in-place ctor
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

        // move constructor
        SBO(SBO&& other) noexcept requires (HasMove<interface_t>) 
        {
            _is_heap = other._is_heap;

            if (_is_heap) {
                // Move unique_ptr
                _heap_obj = std::move(other._heap_obj);
                _ptr = static_cast<interface_t*>(_heap_obj.get());
            } else if (other._ptr) {
                // In-place
                other._ptr->move(reinterpret_cast<interface_t*>(&_buffer));
                _ptr = reinterpret_cast<interface_t*>(&_buffer);
                assert(_ptr != nullptr);
            }
            // Mark other as empty
            other._ptr = nullptr;
        }

        // copy constructor
        SBO(const SBO& other) noexcept requires (HasCloneAndCopy<interface_t>)
        {
            _is_heap = other._is_heap;

            if (_is_heap) {
                // Copy content of unique_ptr
                _heap_obj = other._ptr->clone();
                _ptr = static_cast<interface_t*>(_heap_obj.get());
            } else if (other._ptr) {
                // In-place
                other._ptr->copy(reinterpret_cast<interface_t*>(&_buffer));
                _ptr = reinterpret_cast<interface_t*>(&_buffer);
                assert(_ptr != nullptr);
            }
        }

        // move assignment
        SBO& operator=(SBO&& other) requires (HasMove<interface_t>)
        {
            if (_ptr == other._ptr) return *this;
            if (_is_heap) {
                // Move unique_ptr
                _heap_obj = std::move(other._heap_obj);
                _ptr = static_cast<interface_t*>(_heap_obj.get());
            } else if (other._ptr) {
                // In-place
                other._ptr->move(reinterpret_cast<interface_t*>(&_buffer));
                _ptr = reinterpret_cast<interface_t*>(&_buffer);
                assert(_ptr != nullptr);
            }
            // Mark other as empty
            other._ptr = nullptr;
            return *this;
        }

        // copy assignment
        SBO& operator=(const SBO& other) requires (HasCloneAndCopy<interface_t>)
        {
            if (_ptr == other._ptr) return *this;
            if (_is_heap) {
                // Copy content of unique_ptr
                _heap_obj = other._ptr->clone();
                _ptr = static_cast<interface_t*>(_heap_obj.get());
            } else if (other._ptr) {
                // In-place
                other._ptr->copy(reinterpret_cast<interface_t*>(&_buffer));
                _ptr = reinterpret_cast<interface_t*>(&_buffer);
                assert(_ptr != nullptr);
            }
            return *this;
        }

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
        std::unique_ptr<interface_t> _heap_obj;
        interface_t* _ptr = nullptr;
        bool _is_heap = false;
    };

    /**
     * @brief Implementation class that holds a unique_ptr to the implementation object
     * and provides access to it through the interface.
     * 
     * @tparam ModelTemplate
     */
    template<class InterfaceType, template<class> class ModelTemplate>
    class Implementation
    {
        public:
            using interface_t = InterfaceType;

            Implementation(std::nullptr_t)  = delete;

            //ctor in place 
            template<class ImplType, typename... Args>
            explicit inline Implementation(Args && ... args)
            : _impl(std::in_place_type<ImplType>, std::forward<Args>(args)...)
            {}

            //ctor in place 
            template<class ImplType, typename... Args>
            explicit inline Implementation(std::in_place_type_t<ImplType>, Args && ... args)
            : _impl(std::in_place_type<ImplType>, std::forward<Args>(args)...)
            {}

            //move ctor
            inline Implementation(Implementation && other)
            : _impl{std::move(other._impl)}
            {}

            //move assign
            inline Implementation & operator = (Implementation && other) noexcept {
                if (this != &other) {
                    _impl = std::move(other._impl);
                }
                return *this;
            }

            // copy ctor
            Implementation(const Implementation & other) = delete;
            // copy assign
            Implementation & operator = (const Implementation & other) = delete;
            //dtor
            virtual ~Implementation() = default;

            // access object
            constexpr inline interface_t & operator*() noexcept {return *_impl; }
            // access const object
            constexpr inline const interface_t & operator*() const noexcept{return *_impl; }
            // access object pointer
            constexpr inline interface_t * operator->() noexcept {return _impl.operator->(); }
            // access const object pointer
            constexpr inline const interface_t * operator->() const noexcept{return _impl.operator->(); }

            // access object pointer
            constexpr inline const interface_t* get() const noexcept { return _impl.operator->(); }
            // access const object pointer
            constexpr inline interface_t* get() noexcept { return _impl.operator->(); }

            // checks whether own implementation object
            constexpr explicit operator bool() const noexcept { return bool(_impl);}

        private:
            SBO<InterfaceType, ModelTemplate> _impl;
    };
}