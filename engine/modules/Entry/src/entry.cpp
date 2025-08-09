#include "entry/entry.hpp"

#include <filesystem>
#include <fstream>
#include <chrono>
#include <format>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "version/version.h"

using namespace astre;

namespace astre::entry
{
    AppPaths::AppPaths(const std::filesystem::path& baseDir)
        :   base(baseDir),
            resources(base / "resources"),
            assets(base / "assets"),
            saves(base / "saves"),
            logs(base / "logs")
    {}
}

static const std::string & _getAsciiLogo()
{
    static const std::string logo =
R"(
               _            
     /\       | |           
    /  \   ___| |_ _ __ ___ 
   / /\ \ / __| __| '__/ _ \
  / ____ \\__ \ |_| | |  __/
 /_/    \_\___/\__|_|  \___|
                            
)";
        
    return logo;
}

static std::string _loadBuildTimestamp(const std::filesystem::path & path) 
{
    std::ifstream file(path);
    if (!file.is_open()) return "Unknown";
    std::string timestamp;
    std::getline(file, timestamp);
    return timestamp;
}

static std::string _currentTimestamp()
{
    const auto zt{ std::chrono::zoned_time{
        std::chrono::current_zone(),
        std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now())}
        };
    std::string ts = std::format("{:%F-%H_%M_%S}", zt);
    return ts;
}

static void _configureLogger(std::filesystem::path logs_dir)
{
    std::filesystem::create_directories(logs_dir);

    // Create sinks
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    const std::string log_name = _currentTimestamp() + "-astre.log";
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        (logs_dir / log_name).string(), true);

    // set different log levels per sink
    console_sink->set_level(spdlog::level::debug);
    file_sink->set_level(spdlog::level::debug);
    console_sink->set_pattern("[%T] [%^%l%$][%t] %v");
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S] [%l][%t] %v");
    
    spdlog::logger logger("multi_sink", {console_sink, file_sink});
    logger.set_level(spdlog::level::debug);
    logger.flush_on(spdlog::level::info);

    spdlog::set_default_logger(std::make_shared<spdlog::logger>(logger));
}

int main(int argc, char* argv[])
{
    #ifdef GOOGLE_PROTOBUF_VERSION
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    #endif
    
    entry::AppPaths paths(std::filesystem::path(argv[0]).parent_path());
    _configureLogger(paths.logs);

    spdlog::info("{}", _getAsciiLogo());

    const std::string build_timestamp = _loadBuildTimestamp(paths.base / "build_timestamp");
    spdlog::debug("Build timestamp: {}", build_timestamp);
    spdlog::debug("Version: {}.{}.{}", version::MAJOR_VERSION, version::MINOR_VERSION, version::PATCH_VERSION);

    for(int i = 0; i < argc; ++i)
    {
        spdlog::debug("Argument at [{}] : {}", i, argv[i]);
    }

    const unsigned hardware_cores = std::thread::hardware_concurrency();
    const unsigned used_cores = 4;
    assert(used_cores <= hardware_cores && "Too many cores used");

    // start external entry point
    int return_value;
    {
        // Create a process
        process::Process process(process::createProcess(used_cores));

        asio::co_spawn(process->getExecutionContext(), 
            entry::main(*process, paths), 
            // when main ended set return values
            [&return_value](std::exception_ptr, int r)
            {
                return_value = r;
            }
        );

        // wait for process to end
        process->join();
    }

    spdlog::debug("Program finished with code {}", return_value);

    #ifdef GOOGLE_PROTOBUF_VERSION
    google::protobuf::ShutdownProtobufLibrary();
    #endif

    return return_value;
}