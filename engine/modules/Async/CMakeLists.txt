include("${PROJECT_SOURCE_DIR}/cmake/add_module.cmake")

add_module(NAME "Async"
    INCLUDE_DIRS
        "include"
    SOURCE_DIRS
        "src"
    
    DEPENDENCIES
        asio

        astre::Native
)

# asio-src\asio\include\asio\detail\impl\win_event.ipp(56): warning C6387
target_compile_options(Async PRIVATE /wd6387)
#asio-src\asio\include\asio\detail\impl\win_thread.ipp(49): warning C6258
target_compile_options(Async PRIVATE /wd6258)
#asio-src\asio\include\asio\detail\impl\win_thread.ipp(90): warning C6001
target_compile_options(Async PRIVATE /wd6001)
#asio-src\asio\include\asio\detail\impl\socket_ops.ipp(2550): warning C6255
target_compile_options(Async PRIVATE /wd6255)
#asio-src\asio\include\asio\detail\impl\win_object_handle_service.ipp(105): warning C6031
target_compile_options(Async PRIVATE /wd6031)
#asio-src\asio\include\asio\thread_pool.hpp(659): warning C6313
target_compile_options(Async PRIVATE /wd6313)