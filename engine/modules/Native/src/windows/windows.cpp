#include "native/windows/win.h"

#include <spdlog/spdlog.h>

namespace astre::native{

std::string GetLastErrorAsString() {
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0)
        return "No error"; // No error occurred

    LPSTR messageBuffer = nullptr;

    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errorMessageID,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer, 0, nullptr);

    std::string message(messageBuffer, size);

    // Free the buffer allocated by FormatMessage
    LocalFree(messageBuffer);

    return message;
}

    std::pair<int, std::string> runProcess(const std::string& command, std::vector<std::string> args)
    {
        std::ostringstream oss;

        // Quote the command if it contains spaces
        if (command.find(' ') != std::string::npos)
            oss << "\"" << command << "\"";
        else
            oss << command;

        // Append each argument, quoting if needed
        for (const auto& arg : args)
        {
            oss << ' ';
            if (arg.find(' ') != std::string::npos)
                oss << "\"" << arg << "\"";
            else
                oss << arg;
        }

        const std::string& full_cmd = oss.str();
        
        char cmd_line[1024];

        if (full_cmd.size() >= sizeof(cmd_line))
            throw std::runtime_error("Command line too long");

        strcpy_s(cmd_line, full_cmd.c_str());

        // Create pipe
        HANDLE read_pipe = NULL, write_pipe = NULL;
        SECURITY_ATTRIBUTES sa{};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;

        if (!CreatePipe(&read_pipe, &write_pipe, &sa, 0))
            throw std::runtime_error("Failed to create pipe");

        if (!SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0))
            throw std::runtime_error("Failed to configure pipe");

        // Prepare startup info
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        ZeroMemory(&pi, sizeof(pi));
        si.cb = sizeof(si);
        si.hStdOutput = write_pipe;
        si.hStdError  = write_pipe; // Capture stderr too
        si.dwFlags |= STARTF_USESTDHANDLES;

        if (!CreateProcessA(
            NULL,         // No module name (use command line)
            cmd_line,     // Command line
            NULL,         // Process handle not inheritable
            NULL,         // Thread handle not inheritable
            TRUE,        // Set handle inheritance to FALSE
            0,            // No creation flags
            NULL,         // Use parent's environment block
            NULL,         // Use parent's starting directory 
            &si,          // Pointer to STARTUPINFO structure
            &pi)          // Pointer to PROCESS_INFORMATION structure
        )
        {
            CloseHandle(write_pipe);
            CloseHandle(read_pipe);
            spdlog::error("CreateProcess failed: {}", GetLastErrorAsString());
            throw std::runtime_error("CreateProcess failed: " + GetLastErrorAsString());
        }

        CloseHandle(write_pipe);


        // 6. Read output
        std::string output;
        char buffer[256];
        DWORD bytes_read;

        while (ReadFile(read_pipe, buffer, sizeof(buffer) - 1, &bytes_read, NULL) && bytes_read > 0)
        {
            buffer[bytes_read] = '\0';
            output += buffer;
        }

        // Wait until child process exits
        WaitForSingleObject(pi.hProcess, INFINITE);

        // Read the return code
        DWORD exit_code;
        if (!GetExitCodeProcess(pi.hProcess, &exit_code))
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            throw std::runtime_error("Failed to get process exit code");
        }

        if (exit_code > static_cast<DWORD>(std::numeric_limits<int>::max())) {
            throw std::overflow_error("DWORD value exceeds int range");
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(read_pipe);

        return {static_cast<int>(exit_code), output};
    }
} // namespace astre::native