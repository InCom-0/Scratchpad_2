#pragma once
#include <array>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <fstream>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
#include <knownfolders.h> // FOLDERID_LocalAppData
#include <shlobj.h>       // SHGetKnownFolderPath

#else
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

class TerminalColorQuery {
public:
    struct RGB {
        std::uint8_t   r, g, b;
        constexpr bool operator==(const RGB &) const = default;
    };

    enum class TerminalColorError {
        NoTerminal,
        IoError,
        Timeout,
        ParseError,
        Unsupported
    };
    using Result = std::expected<RGB, TerminalColorError>;

    // query palette index 0..255
    [[nodiscard]] static Result queryPaletteIndex(int index) {
#ifdef _WIN32
        // if (isWindowsTerminal()) { return queryWindowsTerminalJson(index); }
        // else if (isVSCodeTerminal()) { return queryVSCodeJson(index); }
        return queryPaletteIndexWindows(index);
#else
        return queryPaletteIndexPosix(index);
#endif
    }

    [[nodiscard]] static Result queryPaletteIndexFromSgr(int sgrCode) {
        if (auto idx = sgr_to_index(sgrCode)) { return queryPaletteIndex(*idx); }
        return std::unexpected(TerminalColorError::Unsupported);
    }

    [[nodiscard]] static Result queryForeground() {
#ifdef _WIN32
        return queryForegroundWindows();
#else
        return queryForegroundPosix();
#endif
    }

    [[nodiscard]] static Result queryBackground() {
#ifdef _WIN32
        return queryBackgroundWindows();
#else
        return queryBackgroundPosix();
#endif
    }

    [[nodiscard]] static constexpr const char *to_string(TerminalColorError e) noexcept {
        switch (e) {
            case TerminalColorError::NoTerminal:  return "NoTerminal";
            case TerminalColorError::IoError:     return "IoError";
            case TerminalColorError::Timeout:     return "Timeout";
            case TerminalColorError::ParseError:  return "ParseError";
            case TerminalColorError::Unsupported: return "Unsupported";
        }
        return "Unknown";
    }

private:
    // ---------- Common ----------
    [[nodiscard]] static constexpr bool index_valid(int idx) noexcept { return idx >= 0 && idx <= 255; }

    [[nodiscard]] static constexpr std::optional<int> sgr_to_index(int code) noexcept {
        if (code >= 30 && code <= 37) { return code - 30; }
        if (code >= 90 && code <= 97) { return (code - 90) + 8; }
        if (code >= 40 && code <= 47) { return code - 40; }
        if (code >= 100 && code <= 107) { return (code - 100) + 8; }
        return std::nullopt;
    }

    static constexpr inline std::array<RGB, 16> canonical16{
        {RGB{0, 0, 0}, RGB{205, 0, 0}, RGB{0, 205, 0}, RGB{205, 205, 0}, RGB{0, 0, 238}, RGB{205, 0, 205},
         RGB{0, 205, 205}, RGB{229, 229, 229}, RGB{127, 127, 127}, RGB{255, 0, 0}, RGB{0, 255, 0}, RGB{255, 255, 0},
         RGB{92, 92, 255}, RGB{255, 0, 255}, RGB{0, 255, 255}, RGB{255, 255, 255}}};

    static RGB canonicalPalette(int index) noexcept {
        if (index < 16) { return canonical16[index]; }
        // extended indexes just return black if unknown
        return RGB{0, 0, 0};
    }

#ifdef _WIN32
    static RGB colorref_to_rgb(COLORREF c) noexcept {
        return RGB{static_cast<std::uint8_t>(GetRValue(c)), static_cast<std::uint8_t>(GetGValue(c)),
                   static_cast<std::uint8_t>(GetBValue(c))};
    }

    static std::expected<CONSOLE_SCREEN_BUFFER_INFOEX, TerminalColorError> getConsoleInfoEx() noexcept {
        CONSOLE_SCREEN_BUFFER_INFOEX info{};
        info.cbSize = sizeof(info);
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (! hOut || hOut == INVALID_HANDLE_VALUE) { return std::unexpected(TerminalColorError::NoTerminal); }

        if (! GetConsoleScreenBufferInfoEx(hOut, &info)) { return std::unexpected(TerminalColorError::IoError); }

        return info;
    }

    static Result queryPaletteIndexWindows(int index) noexcept {
        if (! index_valid(index)) { return std::unexpected(TerminalColorError::Unsupported); }
        auto infoOrErr = getConsoleInfoEx();
        if (! infoOrErr) { return std::unexpected(infoOrErr.error()); }
        return colorref_to_rgb(infoOrErr->ColorTable[index % 16]); // ColorTable 0–15 only
    }

    static Result queryForegroundWindows() noexcept {
        auto infoOrErr = getConsoleInfoEx();
        if (! infoOrErr) { return std::unexpected(infoOrErr.error()); }
        WORD attr = infoOrErr->wAttributes;
        int  fg   = attr & 0x7;
        if (attr & FOREGROUND_INTENSITY) { fg += 8; }
        if (! index_valid(fg)) { return std::unexpected(TerminalColorError::Unsupported); }
        return colorref_to_rgb(infoOrErr->ColorTable[fg]);
    }

    static Result queryBackgroundWindows() noexcept {
        auto infoOrErr = getConsoleInfoEx();
        if (! infoOrErr) { return std::unexpected(infoOrErr.error()); }
        WORD attr = infoOrErr->wAttributes;
        int  bg   = (attr >> 4) & 0x7;
        if (attr & BACKGROUND_INTENSITY) { bg += 8; }
        if (! index_valid(bg)) { return std::unexpected(TerminalColorError::Unsupported); }
        return colorref_to_rgb(infoOrErr->ColorTable[bg]);
    }

    static std::filesystem::path getLocalAppData() {
        PWSTR                 path = nullptr;
        std::filesystem::path result;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, NULL, &path))) { result = path; }
        if (path) { CoTaskMemFree(path); }
        return result;
    }

    static std::expected<RGB, TerminalColorError> queryWindowsTerminalJson(int index) noexcept {
        auto base = getLocalAppData();
        if (base.empty()) { return std::unexpected(TerminalColorError::NoTerminal); }

        auto jsonPath =
            base / L"Packages" / L"Microsoft.WindowsTerminal_8wekyb3d8bbwe" / L"LocalState" / L"settings.json";
        return parseJsonPalette(jsonPath.generic_string(), index);
    }
    static std::expected<RGB, TerminalColorError> queryVSCodeJson(int index) noexcept {
        auto base = getLocalAppData();
        if (base.empty()) { return std::unexpected(TerminalColorError::NoTerminal); }

        auto jsonPath = base / L"Programs" / L"Microsoft VS Code" / L"resources" / L"app" / L"extensions" / L"theme" /
                        L"colors.json";
        return parseJsonPalette(jsonPath.generic_string(), index);
    }

    static Result parseJsonPalette(const std::string &path, int index) {
        std::ifstream file(path);
        if (! file.is_open()) { return canonical(index); }
        std::stringstream ss;
        ss << file.rdbuf();
        std::string content = ss.str();

        static const std::array<std::string, 16> keys = {
            "ansiBlack",       "ansiRed",           "ansiGreen",       "ansiYellow",
            "ansiBlue",        "ansiMagenta",       "ansiCyan",        "ansiWhite",
            "ansiBrightBlack", "ansiBrightRed",     "ansiBrightGreen", "ansiBrightYellow",
            "ansiBrightBlue",  "ansiBrightMagenta", "ansiBrightCyan",  "ansiBrightWhite"};
        if (index >= 16) { return canonical(index); }
        std::string key = "terminal." + keys[index];

        size_t pos = content.find(key);
        if (pos == std::string::npos) { return canonical(index); }
        size_t hash = content.find('#', pos);
        if (hash == std::string::npos || hash + 6 >= content.size()) { return canonical(index); }

        RGB  out{};
        auto parse2 = [&](char hi, char lo) -> std::uint8_t {
            unsigned v = 0;
            std::from_chars(&hi, &hi + 1, v, 16);
            v            <<= 4;
            unsigned tmp   = 0;
            std::from_chars(&lo, &lo + 1, tmp, 16);
            v |= tmp;
            return static_cast<std::uint8_t>(v);
        };
        out.r = parse2(content[hash + 1], content[hash + 2]);
        out.g = parse2(content[hash + 3], content[hash + 4]);
        out.b = parse2(content[hash + 5], content[hash + 6]);
        return out;
    }

    static Result canonical(int index) { return canonicalPalette(index); }

    static bool isWindowsTerminal() { return std::getenv("WT_SESSION") != nullptr; }
    static bool isVSCodeTerminal() {
        const char *p = std::getenv("TERM_PROGRAM");
        return p && std::string(p) == "vscode";
    }

#else // ---------- POSIX ----------
    struct uniq_fd {
        int fd{-1};
        explicit uniq_fd(int f) : fd(f) {}
        ~uniq_fd() {
            if (fd >= 0) { ::close(fd); }
        }
        uniq_fd(const uniq_fd &)            = delete;
        uniq_fd &operator=(const uniq_fd &) = delete;
        uniq_fd(uniq_fd &&o) noexcept : fd(o.fd) { o.fd = -1; }
        uniq_fd &operator=(uniq_fd &&o) noexcept {
            if (fd >= 0) { ::close(fd); }
            fd   = o.fd;
            o.fd = -1;
            return *this;
        }
        bool valid() const noexcept { return fd >= 0; }
        int  get() const noexcept { return fd; }
    };

    struct termios_guard {
        int     fd{-1};
        termios old{};
        bool    active = false;
        explicit termios_guard(int f) : fd(f) {
            if (fd >= 0 && tcgetattr(fd, &old) == 0) {
                termios raw      = old;
                raw.c_lflag     &= ~(ICANON | ECHO);
                raw.c_cc[VMIN]   = 0;
                raw.c_cc[VTIME]  = 0;
                if (tcsetattr(fd, TCSANOW, &raw) == 0) { active = true; }
            }
        }
        ~termios_guard() {
            if (active) { tcsetattr(fd, TCSANOW, &old); }
        }
        termios_guard(const termios_guard &)            = delete;
        termios_guard &operator=(const termios_guard &) = delete;
    };

    static bool write_all(int fd, const char *data, size_t len) noexcept {
        size_t done = 0;
        while (done < len) {
            ssize_t n = ::write(fd, data + done, len - done);
            if (n < 0) {
                if (errno == EINTR) { continue; }
                return false;
            }
            done += size_t(n);
        }
        return true;
    }

    static std::expected<std::string, TerminalColorError> read_reply_from_tty(int ttyFd, int timeoutMs = 500) noexcept {
        using clock          = std::chrono::steady_clock;
        auto        deadline = clock::now() + std::chrono::milliseconds(timeoutMs);
        std::string buf;
        buf.reserve(64);
        termios_guard guard(ttyFd);
        if (! guard.active) { return std::unexpected(TerminalColorError::IoError); }

        while (clock::now() < deadline) {
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(ttyFd, &rfds);
            auto           remain = std::chrono::duration_cast<std::chrono::microseconds>(deadline - clock::now());
            struct timeval tv{static_cast<time_t>(remain.count() / 1000000),
                              static_cast<suseconds_t>(remain.count() % 1000000)};
            int            r = select(ttyFd + 1, &rfds, nullptr, nullptr, &tv);
            if (r < 0) {
                if (errno == EINTR) { continue; }
                return std::unexpected(TerminalColorError::IoError);
            }
            if (r == 0) {
                continue; // timeout slice
            }
            if (FD_ISSET(ttyFd, &rfds)) {
                char    ch;
                ssize_t n = ::read(ttyFd, &ch, 1);
                if (n < 0) {
                    if (errno == EINTR) { continue; }
                    return std::unexpected(TerminalColorError::IoError);
                }
                if (n == 0) { break; }
                buf.push_back(ch);
                if (buf.back() == '\a') { break; }
                if (buf.size() >= 2 && buf[buf.size() - 2] == '\033' && buf.back() == '\\') { break; }
            }
        }
        if (buf.empty()) { return std::unexpected(TerminalColorError::Timeout); }
        return buf;
    }

    static std::expected<std::string, TerminalColorError> send_osc_and_read(const std::string &osc,
                                                                            int timeoutMs = 500) noexcept {
        // basic terminal detection
        if (! isatty(STDOUT_FILENO) && ! isatty(STDIN_FILENO)) {
            return std::unexpected(TerminalColorError::NoTerminal);
        }

        uniq_fd tty(::open("/dev/tty", O_RDWR | O_NOCTTY));
        if (! tty.valid()) { return std::unexpected(TerminalColorError::NoTerminal); }
        if (! write_all(tty.get(), osc.data(), osc.size())) { return std::unexpected(TerminalColorError::IoError); }
        return read_reply_from_tty(tty.get(), timeoutMs);
    }

    static std::expected<RGB, TerminalColorError> parse_color_from_reply(std::string reply) noexcept {
        // trim trailing \r\n
        while (! reply.empty() && (reply.back() == '\r' || reply.back() == '\n')) { reply.pop_back(); }

        static std::regex rx_rgb(R"(rgb:([0-9A-Fa-f]{1,4})/([0-9A-Fa-f]{1,4})/([0-9A-Fa-f]{1,4}))");
        static std::regex rx_hash(R"(#([0-9A-Fa-f]{6}))");
        std::smatch       m;
        if (std::regex_search(reply, m, rx_rgb)) {
            auto to8 = [](std::string_view s) -> std::uint8_t {
                unsigned v = 0;
                std::from_chars(s.data(), s.data() + s.size(), v, 16);
                if (v > 0xFF) { v /= 257; }
                return static_cast<std::uint8_t>(v);
            };
            return RGB{to8(m[1].str()), to8(m[2].str()), to8(m[3].str())};
        }
        if (std::regex_search(reply, m, rx_hash)) {
            std::string_view hex   = m[1].str();
            auto             from2 = [&](int off) {
                unsigned v = 0;
                std::from_chars(hex.data() + off, hex.data() + off + 2, v, 16);
                return static_cast<std::uint8_t>(v);
            };
            return RGB{from2(0), from2(2), from2(4)};
        }
        return std::unexpected(TerminalColorError::ParseError);
    }

    static std::string make_osc_query(const std::string &body) { return std::string("\033]") + body + '\a'; }

    static Result queryPaletteIndexPosix(int index) noexcept {
        if (! index_valid(index)) { return std::unexpected(TerminalColorError::Unsupported); }
        auto replyOrErr = send_osc_and_read(make_osc_query("4;" + std::to_string(index) + ";?"));
        if (! replyOrErr) { return std::unexpected(replyOrErr.error()); }
        auto rgbOrErr = parse_color_from_reply(*replyOrErr);
        if (! rgbOrErr) { return std::unexpected(rgbOrErr.error()); }
        return *rgbOrErr;
    }
    static Result queryForegroundPosix() noexcept {
        auto replyOrErr = send_osc_and_read(make_osc_query("10;?"));
        if (! replyOrErr) { return std::unexpected(replyOrErr.error()); }
        auto rgbOrErr = parse_color_from_reply(*replyOrErr);
        if (! rgbOrErr) { return std::unexpected(rgbOrErr.error()); }
        return *rgbOrErr;
    }
    static Result queryBackgroundPosix() noexcept {
        auto replyOrErr = send_osc_and_read(make_osc_query("11;?"));
        if (! replyOrErr) { return std::unexpected(replyOrErr.error()); }
        auto rgbOrErr = parse_color_from_reply(*replyOrErr);
        if (! rgbOrErr) { return std::unexpected(rgbOrErr.error()); }
        return *rgbOrErr;
    }
#endif
};
