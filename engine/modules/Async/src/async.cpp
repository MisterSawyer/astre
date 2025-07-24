#include "async/async.hpp"

namespace astre::async
{
    ThreadContext::ThreadContext()
        :   _io_context(1),
            _work_guard(asio::make_work_guard(_io_context)),
            _async_context(_io_context)
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
        close();
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
            
    void ThreadContext::run()
    {
        _io_context.run();
    }
            
    void ThreadContext::poll()
    {
        _io_context.poll();
    }
}