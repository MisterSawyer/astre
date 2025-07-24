#pragma once

#ifndef __unix__
    #error "Error, UNIX not defined"
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <X11/Xlib.h>
#include <GL/glx.h>
#include <unistd.h>  // for pid_t

namespace astre::native
{
    using opengl_context_handle = GLXContext;   // OpenGL context
    using device_context = Display*;           // X11 Display connection
    using console_handle = int;                // File descriptor (e.g., stdout, stderr)
    using window_handle = ::Window;            // X11 Window handle
    using process_handle = pid_t;              // Process ID (Linux uses PIDs, not opaque handles)
}