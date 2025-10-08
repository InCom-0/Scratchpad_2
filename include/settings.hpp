#pragma once

#include <cstdlib>
#include <expected>
#include <filesystem>
#include <fstream>
#include <string>


#include <sqlpp23/sqlite3/sqlite3.h>
#include <sqlpp23/sqlpp23.h>

#if defined(_WIN32)
#include <shlobj.h>
#include <windows.h>

#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <unistd.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

static fs::path getExecutableDir() {
#if defined(_WIN32)
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    return fs::path(buffer).parent_path();
#elif defined(__APPLE__)
    char     buffer[1024];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0) { return fs::path(buffer).parent_path(); }
    else { throw std::runtime_error("Executable path too long"); }
#else
    char    buffer[1024];
    ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer));
    if (count == -1) { throw std::runtime_error("Unable to resolve /proc/self/exe"); }
    return fs::path(std::string(buffer, count)).parent_path();
#endif
}

struct AccessInfo {
    bool readable;
    bool writable;
};

inline std::expected<AccessInfo, std::filesystem::file_type> check_access(const std::filesystem::path &p) {
    namespace fs = std::filesystem;
    std::error_code ec;

    // Resolve symlink (follows by default)
    auto ft = fs::status(p, ec).type();
    if (ec || ft == fs::file_type::not_found) { return std::unexpected(fs::file_type::not_found); }

    // Only allow normal files and directories
    if (ft != fs::file_type::regular && ft != fs::file_type::directory) { return std::unexpected(ft); }

    AccessInfo res{false, false};

    // ---- Regular file ----
    if (ft == fs::file_type::regular) {
        res.readable = std::ifstream(p).good();
        res.writable = std::ofstream(p, std::ios::app).good();
    }

    // ---- Directory ----
    else if (ft == fs::file_type::directory) {
        // Check directory readability by attempting to iterate
        fs::directory_iterator it(p, ec);
        res.readable = ! ec;

        // Check directory writability by trying to create a temp file
        auto          testfile = p / ".fs_test.tmp";
        std::ofstream ofs(testfile);
        if (ofs.good()) {
            res.writable = true;
            ofs.close();
            fs::remove(testfile, ec);
        }
    }

    return res;
}


// 3 Locations get tested:
// 1) Next to executable
// 2) appName_CONFIG_DIR env variable directory
// 3) Default directory expected for a particular system
inline constexpr std::expected<fs::path, std::filesystem::file_type> find_configFile(const std::string &appName,
                                                                                     const std::string &file) {

    fs::path pthToTry = getExecutableDir() / file;

    if (fs::exists(pthToTry)) { return pthToTry; }

    else if (const char *env = std::getenv((appName + "_CONFIG_DIR").c_str())) {
        pthToTry = fs::path(env) / file;
        if (fs::exists(pthToTry)) { return pthToTry; }
    }

    else {
#if defined(_WIN32)
        // 2. Windows AppData
        if (char *appData = std::getenv("APPDATA")) {
            pthToTry = fs::path(appData) / appName / file;
            if (fs::exists(pthToTry)) { return pthToTry; }
        }
#elif defined(__APPLE__)
        // 2. macOS Application Support
        if (const char *home = std::getenv("HOME")) {
            pthToTry = fs::path(home) / "Library" / "Application Support" / appName / fileName;
            if (fs::exists(pthToTry)) { return pthToTry; }
        }
#else
        // 2. Linux/Unix: XDG or ~/.config
        if (const char *xdg = std::getenv("XDG_CONFIG_HOME")) {
            pthToTry = fs::path(xdg) / appName / fileName;
            if (fs::exists(pthToTry)) { return pthToTry; }
        }
        if (const char *home = std::getenv("HOME")) {
            pthToTry = fs::path(xdg) / ".config" / appName / fileName;
            if (fs::exists(pthToTry)) { return pthToTry; }
        }
#endif
    }
    return std::unexpected(fs::file_type::not_found);
}