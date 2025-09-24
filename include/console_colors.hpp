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

#ifdef _WIN32
#include <windows.h>
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

    // ────────────── PUBLIC API ──────────────

    // query palette index 0..255 (returns expected)
    [[nodiscard]] static Result queryPaletteIndex(int index) {
#ifdef _WIN32
        return campbellColor(index);
#else
        return queryPaletteIndexPosix(index);
#endif
    }

    // convenience: always returns something (Campbell on failure)
    [[nodiscard]] static RGB queryPaletteIndex_fb(int index) noexcept {
#ifdef _WIN32
        return campbellColor(index);
#else
        auto res = queryPaletteIndexPosix(index);
        return res ? *res : campbellColor(index);
#endif
    }

    // foreground
    [[nodiscard]] static Result queryForeground() {
#ifdef _WIN32
        return campbellColor(7);
#else
        return queryForegroundPosix();
#endif
    }

    [[nodiscard]] static RGB queryForeground_fb() noexcept {
#ifdef _WIN32
        return campbellColor(7);
#else
        auto res = queryForegroundPosix();
        return res ? *res : campbellColor(7);
#endif
    }

    // background
    [[nodiscard]] static Result queryBackground() {
#ifdef _WIN32
        return campbellColor(0);
#else
        return queryBackgroundPosix();
#endif
    }

    [[nodiscard]] static RGB queryBackground_fb() noexcept {
#ifdef _WIN32
        return campbellColor(0);
#else
        auto res = queryBackgroundPosix();
        return res ? *res : campbellColor(0);
#endif
    }

    // get all 16 colors at once with fallback
    [[nodiscard]] static std::array<RGB, 16> queryAll16_fb() noexcept {
        std::array<RGB, 16> colors{};
        for (int i = 0; i < 16; ++i) {
#ifdef _WIN32
            colors[i] = campbellColor(i);
#else
            auto res  = queryPaletteIndexPosix(i);
            colors[i] = res ? *res : campbellColor(i);
#endif
        }
        return colors;
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

    // direct Campbell color
    [[nodiscard]] static RGB campbellColor(int index) noexcept {
        if (! index_valid(index)) { return RGB{0, 0, 0}; }
        return campbell16[index % 16];
    }

private:
    // ────────────── INTERNAL ──────────────

    [[nodiscard]] static constexpr bool index_valid(int idx) noexcept { return idx >= 0 && idx <= 255; }

    // Default Windows Terminal Campbell palette
    static constexpr inline std::array<RGB, 16> campbell16{
        RGB{12, 12, 12},    RGB{197, 15, 31},  RGB{19, 161, 14},  RGB{193, 156, 0},
        RGB{0, 55, 218},    RGB{136, 23, 152}, RGB{58, 150, 221}, RGB{204, 204, 204},
        RGB{118, 118, 118}, RGB{231, 72, 86},  RGB{22, 198, 12},  RGB{249, 241, 165},
        RGB{59, 120, 255},  RGB{180, 0, 158},  RGB{97, 214, 214}, RGB{242, 242, 242}};

#ifdef _WIN32
    // So far nothing here
    // In the future might have some implementation once Windows terminal reports actually used colors with OSC 4

#else
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
        if (! isatty(STDOUT_FILENO) && ! isatty(STDIN_FILENO)) {
            return std::unexpected(TerminalColorError::NoTerminal);
        }
        uniq_fd tty(::open("/dev/tty", O_RDWR | O_NOCTTY));
        if (! tty.valid()) { return std::unexpected(TerminalColorError::NoTerminal); }
        if (! write_all(tty.get(), osc.data(), osc.size())) { return std::unexpected(TerminalColorError::IoError); }
        return read_reply_from_tty(tty.get(), timeoutMs);
    }

    static std::expected<RGB, TerminalColorError> parse_color_from_reply(std::string reply) noexcept {
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

    static std::string make_osc_query(const std::string &body) { return "\033]" + body + '\a'; }

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
