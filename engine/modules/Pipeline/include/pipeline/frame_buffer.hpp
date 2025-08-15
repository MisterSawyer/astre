#pragma once
#include <array>
#include <utility>

namespace astre::pipeline
{
    template <typename T, std::size_t Count = 3>
    class FramesBuffer : public std::array<T, Count>
    {
    public:        
        static_assert(Count >= 2, "Need at least 2 frames for interpolation");

        T& current()                    { return std::array<T, Count>::at(_index); }
        const T& previous() const       { return std::array<T, Count>::at((_index + Count - 1) % Count); }
        const T& beforePrevious() const { return std::array<T, Count>::at((_index + Count - 2) % Count); }

        void rotate() { _index = (_index + 1) % Count; }
        std::size_t index() const { return _index; }

    private:
        std::size_t _index = 0;
    };
}