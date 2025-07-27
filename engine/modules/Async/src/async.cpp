#include "async/async.hpp"

namespace astre::async
{
    ThreadContext::ThreadContext()
        :   asio::io_context(1),
            _work_guard(asio::make_work_guard(*this)),
            _async_context(*this)
    {}

    ThreadContext::~ThreadContext()
    {
        join();
    }

    asio::awaitable<void> ThreadContext::ensureOnStrand() const
    {
        return _async_context.ensureOnStrand();
    }

    void ThreadContext::join()
    {
        if(_thread.joinable() == false)return;
        _thread.join();
    }

    void ThreadContext::close()
    {
        if(_work_guard.owns_work() == false)return;
        _work_guard.reset();
    }
        
    bool ThreadContext::running() const
    {
        if(_work_guard.owns_work() == false)return false;
        if(_thread.joinable() == false)return false;
        return true;
    }
}