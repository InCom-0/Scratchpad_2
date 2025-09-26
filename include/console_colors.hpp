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

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace incom::standard::console {
namespace color {

struct INCCC_RBG {
    std::uint8_t   r, g, b;
    constexpr bool operator==(const INCCC_RBG &) const = default;
};

using palette16  = std::array<INCCC_RBG, 16>;
using palette256 = std::array<INCCC_RBG, 256>;


enum class TerminalColorError {
    NoTerminal,
    IoError,
    Timeout,
    ParseError,
    Unsupported
};

namespace color_schemes {
struct scheme16 {
    palette16 palette;
    INCCC_RBG foreground;
    INCCC_RBG backgrond;
    INCCC_RBG cursor;
    INCCC_RBG selection;
};
struct scheme256 {
    palette256 palette;
    INCCC_RBG  foreground;
    INCCC_RBG  backgrond;
    INCCC_RBG  cursor;
    INCCC_RBG  selection;
};

namespace windows_terminal {
inline constexpr scheme16 campbell{
    {{INCCC_RBG{12, 12, 12}, INCCC_RBG{197, 15, 31}, INCCC_RBG{19, 161, 14}, INCCC_RBG{193, 156, 0},
      INCCC_RBG{0, 55, 218}, INCCC_RBG{136, 23, 152}, INCCC_RBG{58, 150, 221}, INCCC_RBG{204, 204, 204},
      INCCC_RBG{118, 118, 118}, INCCC_RBG{231, 72, 86}, INCCC_RBG{22, 198, 12}, INCCC_RBG{249, 241, 165},
      INCCC_RBG{59, 120, 255}, INCCC_RBG{180, 0, 158}, INCCC_RBG{97, 214, 214}, INCCC_RBG{242, 242, 242}}},
    {204, 204, 204},
    {12, 12, 12},
    {255, 255, 255},
    {255, 255, 255}};

inline constexpr scheme16 dimidium{
    {{INCCC_RBG{0, 0, 0}, INCCC_RBG{204, 36, 29}, INCCC_RBG{80, 204, 80}, INCCC_RBG{204, 204, 29},
      INCCC_RBG{29, 80, 204}, INCCC_RBG{204, 29, 204}, INCCC_RBG{29, 204, 204}, INCCC_RBG{204, 204, 204},
      INCCC_RBG{102, 102, 102}, INCCC_RBG{204, 102, 102}, INCCC_RBG{102, 204, 102}, INCCC_RBG{204, 204, 102},
      INCCC_RBG{102, 102, 204}, INCCC_RBG{204, 102, 204}, INCCC_RBG{102, 204, 204}, INCCC_RBG{204, 204, 204}}},
    {204, 204, 204}, // foreground
    {0, 0, 0},       // background
    {204, 204, 204}, // cursor
    {204, 204, 204}  // selection
};

inline constexpr scheme16 dark_plus{
    {{INCCC_RBG{0, 0, 0}, INCCC_RBG{198, 47, 55}, INCCC_RBG{55, 190, 120}, INCCC_RBG{226, 232, 34},
      INCCC_RBG{57, 110, 199}, INCCC_RBG{184, 53, 188}, INCCC_RBG{59, 167, 204}, INCCC_RBG{229, 229, 229},
      INCCC_RBG{102, 102, 102}, INCCC_RBG{233, 74, 81}, INCCC_RBG{69, 211, 138}, INCCC_RBG{242, 248, 74},
      INCCC_RBG{78, 138, 233}, INCCC_RBG{210, 106, 214}, INCCC_RBG{73, 183, 218}, INCCC_RBG{229, 229, 229}}},
    {204, 204, 204}, // foreground
    {30, 30, 30},    // background
    {204, 204, 204}, // cursor
    {204, 204, 204}  // selection
};

inline constexpr scheme16 vintage{
    {{INCCC_RBG{0, 0, 0}, INCCC_RBG{128, 0, 0}, INCCC_RBG{0, 128, 0}, INCCC_RBG{128, 128, 0}, INCCC_RBG{0, 0, 128},
      INCCC_RBG{128, 0, 128}, INCCC_RBG{0, 128, 128}, INCCC_RBG{192, 192, 192}, INCCC_RBG{128, 128, 128},
      INCCC_RBG{255, 0, 0}, INCCC_RBG{0, 255, 0}, INCCC_RBG{255, 255, 0}, INCCC_RBG{0, 0, 255}, INCCC_RBG{255, 0, 255},
      INCCC_RBG{0, 255, 255}, INCCC_RBG{255, 255, 255}}},
    {192, 192, 192}, // foreground
    {0, 0, 0},       // background
    {192, 192, 192}, // cursor (fallback)
    {192, 192, 192}  // selection (fallback)
};

inline constexpr scheme16 ottosson{
    {{INCCC_RBG{0, 38, 26}, INCCC_RBG{255, 0, 0}, INCCC_RBG{0, 255, 0}, INCCC_RBG{255, 255, 0}, INCCC_RBG{0, 0, 255},
      INCCC_RBG{255, 0, 255}, INCCC_RBG{0, 255, 255}, INCCC_RBG{255, 255, 255}, INCCC_RBG{85, 85, 85},
      INCCC_RBG{255, 85, 85}, INCCC_RBG{85, 255, 85}, INCCC_RBG{255, 255, 85}, INCCC_RBG{85, 85, 255},
      INCCC_RBG{255, 85, 255}, INCCC_RBG{85, 255, 255}, INCCC_RBG{255, 255, 255}}},
    {255, 255, 255}, // foreground
    {0, 38, 26},     // background
    {255, 255, 255}, // cursor
    {0, 85, 51}      // selection (fallback greenish)
};

inline constexpr scheme16 one_half_dark{
    {{INCCC_RBG{40, 44, 52}, INCCC_RBG{224, 108, 117}, INCCC_RBG{152, 195, 121}, INCCC_RBG{229, 192, 123},
      INCCC_RBG{97, 175, 239}, INCCC_RBG{198, 120, 221}, INCCC_RBG{86, 182, 194}, INCCC_RBG{220, 223, 228},
      INCCC_RBG{92, 99, 112}, INCCC_RBG{224, 108, 117}, INCCC_RBG{152, 195, 121}, INCCC_RBG{229, 192, 123},
      INCCC_RBG{97, 175, 239}, INCCC_RBG{198, 120, 221}, INCCC_RBG{86, 182, 194}, INCCC_RBG{220, 223, 228}}},
    {220, 223, 228},
    {40, 44, 52},
    {220, 223, 228},
    {220, 223, 228}};

inline constexpr scheme16 one_half_light{
    {{INCCC_RBG{250, 250, 250}, INCCC_RBG{224, 108, 117}, INCCC_RBG{152, 195, 121}, INCCC_RBG{184, 173, 104},
      INCCC_RBG{97, 175, 239}, INCCC_RBG{198, 120, 221}, INCCC_RBG{86, 182, 194}, INCCC_RBG{56, 58, 66},
      INCCC_RBG{153, 153, 153}, INCCC_RBG{224, 108, 117}, INCCC_RBG{152, 195, 121}, INCCC_RBG{184, 173, 104},
      INCCC_RBG{97, 175, 239}, INCCC_RBG{198, 120, 221}, INCCC_RBG{86, 182, 194}, INCCC_RBG{56, 58, 66}}},
    {56, 58, 66},
    {250, 250, 250},
    {56, 58, 66},
    {56, 58, 66}};

inline constexpr scheme16 solarized_dark{
    {{INCCC_RBG{0, 43, 54}, INCCC_RBG{220, 50, 47}, INCCC_RBG{133, 153, 0}, INCCC_RBG{181, 137, 0},
      INCCC_RBG{38, 139, 210}, INCCC_RBG{211, 54, 130}, INCCC_RBG{42, 161, 152}, INCCC_RBG{238, 232, 213},
      INCCC_RBG{88, 110, 117}, INCCC_RBG{203, 75, 22}, INCCC_RBG{88, 110, 117}, INCCC_RBG{101, 123, 131},
      INCCC_RBG{131, 148, 150}, INCCC_RBG{108, 113, 196}, INCCC_RBG{147, 161, 161}, INCCC_RBG{253, 246, 227}}},
    {131, 148, 150},
    {0, 43, 54},
    {131, 148, 150},
    {131, 148, 150}};

inline constexpr scheme16 solarized_light{
    {{INCCC_RBG{253, 246, 227}, INCCC_RBG{220, 50, 47}, INCCC_RBG{133, 153, 0}, INCCC_RBG{181, 137, 0},
      INCCC_RBG{38, 139, 210}, INCCC_RBG{211, 54, 130}, INCCC_RBG{42, 161, 152}, INCCC_RBG{7, 54, 66},
      INCCC_RBG{238, 232, 213}, INCCC_RBG{203, 75, 22}, INCCC_RBG{88, 110, 117}, INCCC_RBG{101, 123, 131},
      INCCC_RBG{131, 148, 150}, INCCC_RBG{108, 113, 196}, INCCC_RBG{147, 161, 161}, INCCC_RBG{0, 43, 54}}},
    {101, 123, 131},
    {253, 246, 227},
    {101, 123, 131},
    {101, 123, 131}};

inline constexpr scheme16 tango_dark{
    {{INCCC_RBG{0, 0, 0}, INCCC_RBG{204, 0, 0}, INCCC_RBG{78, 154, 6}, INCCC_RBG{196, 160, 0}, INCCC_RBG{52, 101, 164},
      INCCC_RBG{117, 80, 123}, INCCC_RBG{6, 152, 154}, INCCC_RBG{211, 215, 207}, INCCC_RBG{85, 87, 83},
      INCCC_RBG{239, 41, 41}, INCCC_RBG{138, 226, 52}, INCCC_RBG{252, 233, 79}, INCCC_RBG{114, 159, 207},
      INCCC_RBG{173, 127, 168}, INCCC_RBG{52, 226, 226}, INCCC_RBG{238, 238, 236}}},
    {238, 238, 238},
    {34, 34, 34},
    {238, 238, 238},
    {238, 238, 238}};

inline constexpr scheme16 tango_light{
    {{INCCC_RBG{0, 0, 0}, INCCC_RBG{204, 0, 0}, INCCC_RBG{78, 154, 6}, INCCC_RBG{196, 160, 0}, INCCC_RBG{52, 101, 164},
      INCCC_RBG{117, 80, 123}, INCCC_RBG{6, 152, 154}, INCCC_RBG{211, 215, 207}, INCCC_RBG{85, 87, 83},
      INCCC_RBG{239, 41, 41}, INCCC_RBG{138, 226, 52}, INCCC_RBG{252, 233, 79}, INCCC_RBG{114, 159, 207},
      INCCC_RBG{173, 127, 168}, INCCC_RBG{52, 226, 226}, INCCC_RBG{238, 238, 236}}},
    {56, 58, 66},
    {255, 255, 255},
    {56, 58, 66},
    {56, 58, 66}};

inline constexpr scheme16 cga{
    {{INCCC_RBG{0, 0, 0}, INCCC_RBG{0, 0, 170}, INCCC_RBG{0, 170, 0}, INCCC_RBG{0, 170, 170}, INCCC_RBG{170, 0, 0},
      INCCC_RBG{170, 0, 170}, INCCC_RBG{170, 85, 0}, INCCC_RBG{170, 170, 170}, INCCC_RBG{85, 85, 85},
      INCCC_RBG{85, 85, 255}, INCCC_RBG{85, 255, 85}, INCCC_RBG{85, 255, 255}, INCCC_RBG{255, 85, 85},
      INCCC_RBG{255, 85, 255}, INCCC_RBG{255, 255, 85}, INCCC_RBG{255, 255, 255}}},
    {170, 170, 170}, // foreground
    {0, 0, 0},       // background
    {170, 170, 170}, // cursor
    {85, 85, 85}     // selection (fallback)
};

inline constexpr scheme16 ibm_5153{
    cga.palette,     // palette identical to CGA
    {170, 170, 170}, // foreground
    {0, 0, 0},       // background
    {170, 170, 170}, // cursor
    {85, 85, 85}     // selection (fallback)
};

inline constexpr scheme16 campbell_powershell{
    {{INCCC_RBG{12, 12, 12}, INCCC_RBG{197, 15, 31}, INCCC_RBG{19, 161, 14}, INCCC_RBG{193, 156, 0},
      INCCC_RBG{0, 55, 218}, INCCC_RBG{136, 23, 152}, INCCC_RBG{58, 150, 221}, INCCC_RBG{204, 204, 204},
      INCCC_RBG{118, 118, 118}, INCCC_RBG{231, 72, 86}, INCCC_RBG{22, 198, 12}, INCCC_RBG{249, 241, 165},
      INCCC_RBG{59, 120, 255}, INCCC_RBG{180, 0, 158}, INCCC_RBG{97, 214, 214}, INCCC_RBG{242, 242, 242}}},
    {204, 204, 204}, // foreground
    {1, 36, 86},     // background
    {255, 255, 255}, // cursor
    {255, 255, 255}  // selection
};


} // namespace windows_terminal
} // namespace color_schemes


class TerminalColorQuery {
public:
    // ────────────── PUBLIC API ──────────────
    using Result = std::expected<INCCC_RBG, TerminalColorError>;

    // query palette index 0..255 (returns expected)
    [[nodiscard]] static constexpr Result queryPaletteIndex(int index) {
#ifdef _WIN32
        return campbellColor(index);
#else
        return queryPaletteIndexPosix(index);
#endif
    }

    // convenience: always returns something (Campbell on failure)
    [[nodiscard]] static constexpr INCCC_RBG queryPaletteIndex_fb(int index) noexcept {
#ifdef _WIN32
        return campbellColor(index);
#else
        auto res = queryPaletteIndexPosix(index);
        return res ? *res : campbellColor(index);
#endif
    }

    // foreground
    [[nodiscard]] static constexpr Result queryForeground() {
#ifdef _WIN32
        return campbellColor(7);
#else
        return queryForegroundPosix();
#endif
    }

    [[nodiscard]] static constexpr INCCC_RBG queryForeground_fb() noexcept {
#ifdef _WIN32
        return campbellColor(7);
#else
        auto res = queryForegroundPosix();
        return res ? *res : campbellColor(7);
#endif
    }

    // background
    [[nodiscard]] static constexpr Result queryBackground() {
#ifdef _WIN32
        return campbellColor(0);
#else
        return queryBackgroundPosix();
#endif
    }

    [[nodiscard]] static constexpr INCCC_RBG queryBackground_fb() noexcept {
#ifdef _WIN32
        return campbellColor(0);
#else
        auto res = queryBackgroundPosix();
        return res ? *res : campbellColor(0);
#endif
    }

    // get all 16 colors at once with fallback
    [[nodiscard]] static constexpr palette16 queryAll16_fb() noexcept {
        palette16 colors{};
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

    [[nodiscard]] static constexpr std::string_view to_string(TerminalColorError e) noexcept {
        using namespace std::literals;
        switch (e) {
            case TerminalColorError::NoTerminal:  return "NoTerminal"sv;
            case TerminalColorError::IoError:     return "IoError"sv;
            case TerminalColorError::Timeout:     return "Timeout"sv;
            case TerminalColorError::ParseError:  return "ParseError"sv;
            case TerminalColorError::Unsupported: return "Unsupported"sv;
        }
        return "Unknown"sv;
    }

    // direct Campbell color
    [[nodiscard]] static constexpr INCCC_RBG campbellColor(int index) noexcept {
        if (! index16_valid(index)) { return INCCC_RBG{255, 255, 255}; }
        return color_schemes::windows_terminal::campbell.palette[index];
    }

private:
    // ────────────── INTERNAL ──────────────

    [[nodiscard]] static constexpr bool index256_valid(int idx) noexcept { return idx >= 0 && idx <= 255; }
    [[nodiscard]] static constexpr bool index16_valid(int idx) noexcept { return idx >= 0 && idx <= 15; }

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

    static constexpr bool write_all(int fd, const char *data, size_t len) noexcept {
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

    static constexpr std::expected<std::string, TerminalColorError> read_reply_from_tty(int ttyFd,
                                                                                        int timeoutMs = 500) noexcept {
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

    static constexpr std::expected<std::string, TerminalColorError> send_osc_and_read(const std::string &osc,
                                                                                      int timeoutMs = 500) noexcept {
        if (! isatty(STDOUT_FILENO) && ! isatty(STDIN_FILENO)) {
            return std::unexpected(TerminalColorError::NoTerminal);
        }
        uniq_fd tty(::open("/dev/tty", O_RDWR | O_NOCTTY));
        if (! tty.valid()) { return std::unexpected(TerminalColorError::NoTerminal); }
        if (! write_all(tty.get(), osc.data(), osc.size())) { return std::unexpected(TerminalColorError::IoError); }
        return read_reply_from_tty(tty.get(), timeoutMs);
    }

    static constexpr std::expected<TC_RBG, TerminalColorError> parse_color_from_reply(std::string reply) noexcept {
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
            return TC_RBG{to8(m[1].str()), to8(m[2].str()), to8(m[3].str())};
        }
        if (std::regex_search(reply, m, rx_hash)) {
            std::string_view hex   = m[1].str();
            auto             from2 = [&](int off) {
                unsigned v = 0;
                std::from_chars(hex.data() + off, hex.data() + off + 2, v, 16);
                return static_cast<std::uint8_t>(v);
            };
            return TC_RBG{from2(0), from2(2), from2(4)};
        }
        return std::unexpected(TerminalColorError::ParseError);
    }

    static constexpr std::string make_osc_query(const std::string &body) { return "\033]" + body + '\a'; }

    static constexpr Result queryPaletteIndexPosix(int index) noexcept {
        if (! index256_valid(index)) { return std::unexpected(TerminalColorError::Unsupported); }
        auto replyOrErr = send_osc_and_read(make_osc_query("4;" + std::to_string(index) + ";?"));
        if (! replyOrErr) { return std::unexpected(replyOrErr.error()); }
        auto rgbOrErr = parse_color_from_reply(*replyOrErr);
        if (! rgbOrErr) { return std::unexpected(rgbOrErr.error()); }
        return *rgbOrErr;
    }

    static constexpr Result queryForegroundPosix() noexcept {
        auto replyOrErr = send_osc_and_read(make_osc_query("10;?"));
        if (! replyOrErr) { return std::unexpected(replyOrErr.error()); }
        auto rgbOrErr = parse_color_from_reply(*replyOrErr);
        if (! rgbOrErr) { return std::unexpected(rgbOrErr.error()); }
        return *rgbOrErr;
    }

    static constexpr Result queryBackgroundPosix() noexcept {
        auto replyOrErr = send_osc_and_read(make_osc_query("11;?"));
        if (! replyOrErr) { return std::unexpected(replyOrErr.error()); }
        auto rgbOrErr = parse_color_from_reply(*replyOrErr);
        if (! rgbOrErr) { return std::unexpected(rgbOrErr.error()); }
        return *rgbOrErr;
    }
#endif
};
} // namespace color
} // namespace incom::standard::console
