#pragma once

#include <iostream>
#include <string>
#include <cstdlib>
#include <array>
#include <optional>
#include <fstream>
#include <sstream>
#include <expected>
#include <vector>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#include <memory>
#endif

#ifdef __unix__
#include <unistd.h>
#include <sys/ioctl.h>
#endif

class TerminalColorQuery {
public:
  struct RGB {
    int r;
    int g;
    int b;
  };
  enum class TerminalColorError {
    NoTerminal = 0,
    IoError,
    Timeout,
    ParseError,
    Unsupported
  };
  using Result = std::expected<RGB, TerminalColorError>;

  // Query palette index 0..15 (POSIX will ask OSC 4;index;?, Windows will read
  // ColorTable[index]) Returns unexpected(TerminalColorError::ParseError) if
  // reply can't be parsed.
  static Result queryPaletteIndex(int index) {
#ifdef _WIN32
    return queryPaletteIndexWindows(index);
#else
    return queryPaletteIndexPosix(index);
#endif
  }

  // --- New overload: query by SGR code ---
  // Supports: 30–37, 90–97 (foreground), 40–47, 100–107 (background)
  static Result queryPaletteIndexFromSgr(int sgrCode) {
    int idx = sgr_to_index(sgrCode);
    if (idx == -1)
      return std::unexpected(TerminalColorError::Unsupported);
    return queryPaletteIndex(idx);
  }

  // Query the current foreground RGB (POSIX: OSC 10;?; Windows: read attributes
  // -> palette index)
  static Result queryForeground() {
#ifdef _WIN32
    return queryForegroundWindows();
#else
    return queryForegroundPosix();
#endif
  }

  // Query the current background RGB (POSIX: OSC 11;?; Windows: read attributes
  // -> palette index)
  static Result queryBackground() {
#ifdef _WIN32
    return queryBackgroundWindows();
#else
    return queryBackgroundPosix();
#endif
  }

  // helper
  static const char *to_string(TerminalColorError e) noexcept {
    switch (e) {
    case TerminalColorError::NoTerminal:
      return "NoTerminal";
    case TerminalColorError::IoError:
      return "IoError";
    case TerminalColorError::Timeout:
      return "Timeout";
    case TerminalColorError::ParseError:
      return "ParseError";
    case TerminalColorError::Unsupported:
      return "Unsupported";
    }
    return "Unknown";
  }

private:
  // ---------------- Common utils ----------------
  static bool index_valid(int idx) noexcept { return idx >= 0 && idx <= 15; }

  // Map SGR codes to palette indexes (0–15)
  static int sgr_to_index(int code) noexcept {
    // Foreground normal
    if (code >= 30 && code <= 37)
      return code - 30;
    // Foreground bright
    if (code >= 90 && code <= 97)
      return (code - 90) + 8;
    // Background normal
    if (code >= 40 && code <= 47)
      return code - 40;
    // Background bright
    if (code >= 100 && code <= 107)
      return (code - 100) + 8;
    return -1; // unsupported
  }

#ifdef _WIN32

  // Convert COLORREF to RGB
  static RGB colorref_to_rgb(COLORREF c) noexcept {
    return RGB{static_cast<int>(GetRValue(c)), static_cast<int>(GetGValue(c)),
               static_cast<int>(GetBValue(c))};
  }

  // Try to get CONSOLE_SCREEN_BUFFER_INFOEX; returns unexpected on failure
  static std::expected<CONSOLE_SCREEN_BUFFER_INFOEX, TerminalColorError>
  getConsoleInfoEx() noexcept {
    CONSOLE_SCREEN_BUFFER_INFOEX info{};
    info.cbSize = sizeof(info);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE || hOut == nullptr)
      return std::unexpected(TerminalColorError::NoTerminal);

    // Some environments give handles that are not console handles;
    // GetConsoleScreenBufferInfoEx will fail in that case.
    if (!GetConsoleScreenBufferInfoEx(hOut, &info)) {
      // Try AttachConsole to parent's console and retry once
      if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        // re-obtain handle and try again
        FreeConsole(); // we'll reattach only temporarily; continue to indicate
                       // error if still failing
        // Note: Attaching temporarily can be noisy; best-effort only.
      }
      // final attempt: try with current handle again
      if (!GetConsoleScreenBufferInfoEx(hOut, &info)) {
        return std::unexpected(TerminalColorError::IoError);
      }
    }
    return info;
  }

  static Result queryPaletteIndexWindows(int index) noexcept {
    if (!index_valid(index))
      return std::unexpected(TerminalColorError::Unsupported);
    auto infoOrErr = getConsoleInfoEx();
    if (!infoOrErr)
      return std::unexpected(infoOrErr.error());

    const CONSOLE_SCREEN_BUFFER_INFOEX &info = *infoOrErr;
    // ColorTable is an array of 16 COLORREFs
    COLORREF cref = info.ColorTable[index];
    return colorref_to_rgb(cref);
  }

  static Result queryForegroundWindows() noexcept {
    auto infoOrErr = getConsoleInfoEx();
    if (!infoOrErr)
      return std::unexpected(infoOrErr.error());
    const CONSOLE_SCREEN_BUFFER_INFOEX &info = *infoOrErr;
    WORD attr = info.wAttributes;
    // foreground index is low 3 bits
    int fg = attr & 0x7;
    // intensity -> high-bright (add 8)
    if (attr & FOREGROUND_INTENSITY)
      fg += 8;
    if (!index_valid(fg))
      return std::unexpected(TerminalColorError::Unsupported);
    return colorref_to_rgb(info.ColorTable[fg]);
  }

  static Result queryBackgroundWindows() noexcept {
    auto infoOrErr = getConsoleInfoEx();
    if (!infoOrErr)
      return std::unexpected(infoOrErr.error());
    const CONSOLE_SCREEN_BUFFER_INFOEX &info = *infoOrErr;
    WORD attr = info.wAttributes;
    int bg = (attr >> 4) & 0x7;
    // background intensity bit is 0x80 (BACKGROUND_INTENSITY)
    if (attr & BACKGROUND_INTENSITY)
      bg += 8;
    if (!index_valid(bg))
      return std::unexpected(TerminalColorError::Unsupported);
    return colorref_to_rgb(info.ColorTable[bg]);
  }

#else // POSIX branch

  // Build OSC sequence
  static std::string make_osc_query(const std::string &body) {
    return std::string("\033]") + body + '\a'; // BEL terminated
  }

  // RAII fd wrapper
  struct uniq_fd {
    int fd{-1};
    explicit uniq_fd(int fd_) : fd(fd_) {}
    ~uniq_fd() {
      if (fd >= 0)
        ::close(fd);
    }
    uniq_fd(const uniq_fd &) = delete;
    uniq_fd &operator=(const uniq_fd &) = delete;
    uniq_fd(uniq_fd &&o) noexcept : fd(o.fd) { o.fd = -1; }
    uniq_fd &operator=(uniq_fd &&o) noexcept {
      if (fd >= 0)
        ::close(fd);
      fd = o.fd;
      o.fd = -1;
      return *this;
    }
    bool valid() const noexcept { return fd >= 0; }
    int get() const noexcept { return fd; }
  };

  // termios guard: set raw (no echo, non-canonical) for the fd, restore on
  // destruction
  struct termios_guard {
    int fd{-1};
    termios old{};
    bool active{false};
    termios_guard() = default;
    explicit termios_guard(int fd_) : fd(fd_) {
      if (fd >= 0 && tcgetattr(fd, &old) == 0) {
        termios raw = old;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 1; // 0.1s
        if (tcsetattr(fd, TCSANOW, &raw) == 0)
          active = true;
      }
    }
    ~termios_guard() {
      if (active)
        tcsetattr(fd, TCSANOW, &old);
    }
    termios_guard(const termios_guard &) = delete;
    termios_guard &operator=(const termios_guard &) = delete;
  };

  // write all bytes
  static bool write_all(int fd, const char *data, size_t len) noexcept {
    size_t wrote = 0;
    while (wrote < len) {
      ssize_t n = ::write(fd, data + wrote, len - wrote);
      if (n < 0) {
        if (errno == EINTR)
          continue;
        return false;
      }
      wrote += static_cast<size_t>(n);
    }
    return true;
  }

  // read reply from the tty fd with a total timeout (ms)
  static std::expected<std::string, TerminalColorError>
  read_reply_from_tty(int ttyFd, int timeoutMs = 500) noexcept {
    using clock = std::chrono::steady_clock;
    const auto deadline = clock::now() + std::chrono::milliseconds(timeoutMs);
    std::string buf;
    buf.reserve(64);
    termios_guard guard(ttyFd); // restore on exit if set
    if (!guard.active)
      return std::unexpected(TerminalColorError::IoError);

    while (clock::now() < deadline) {
      fd_set rfds;
      FD_ZERO(&rfds);
      FD_SET(ttyFd, &rfds);
      auto remain = std::chrono::duration_cast<std::chrono::microseconds>(
          deadline - clock::now());
      if (remain.count() <= 0)
        break;
      struct timeval tv;
      tv.tv_sec = static_cast<time_t>(remain.count() / 1000000);
      tv.tv_usec = static_cast<suseconds_t>(remain.count() % 1000000);
      int r = select(ttyFd + 1, &rfds, nullptr, nullptr, &tv);
      if (r < 0) {
        if (errno == EINTR)
          continue;
        return std::unexpected(TerminalColorError::IoError);
      }
      if (r == 0)
        break; // timeout slice
      if (FD_ISSET(ttyFd, &rfds)) {
        char ch;
        ssize_t n = ::read(ttyFd, &ch, 1);
        if (n < 0) {
          if (errno == EINTR)
            continue;
          return std::unexpected(TerminalColorError::IoError);
        }
        if (n == 0)
          break;
        buf.push_back(ch);
        if (buf.size() >= 1 && buf.back() == '\a')
          break;
        if (buf.size() >= 2 && buf[buf.size() - 2] == '\033' &&
            buf.back() == '\\')
          break;
      }
    }

    if (buf.empty())
      return std::unexpected(TerminalColorError::Timeout);
    return buf;
  }

  // send OSC query to /dev/tty and read reply
  static std::expected<std::string, TerminalColorError>
  send_osc_and_read(const std::string &osc, int timeoutMs = 500) noexcept {
    uniq_fd tty(::open("/dev/tty", O_RDWR | O_NOCTTY));
    if (!tty.valid())
      return std::unexpected(TerminalColorError::NoTerminal);

    if (!write_all(tty.get(), osc.data(), osc.size()))
      return std::unexpected(TerminalColorError::IoError);
    (void)::fsync(tty.get());

    return read_reply_from_tty(tty.get(), timeoutMs);
  }

  // parse rgb:hhhh/.... or #RRGGBB
  static std::expected<RGB, TerminalColorError>
  parse_color_from_reply(const std::string &reply) noexcept {
    static const std::regex rx_rgb(
        R"(rgb:([0-9A-Fa-f]{1,4})/([0-9A-Fa-f]{1,4})/([0-9A-Fa-f]{1,4}))");
    static const std::regex rx_hash(R"(#([0-9A-Fa-f]{6}))");
    std::smatch m;
    if (std::regex_search(reply, m, rx_rgb)) {
      try {
        auto to8 = [](const std::string &s) -> int {
          int v = std::stoi(s, nullptr, 16);
          return v > 0xFF ? (v / 257) : v;
        };
        int r = to8(m[1].str());
        int g = to8(m[2].str());
        int b = to8(m[3].str());
        return RGB{r, g, b};
      } catch (...) {
        return std::unexpected(TerminalColorError::ParseError);
      }
    }
    if (std::regex_search(reply, m, rx_hash)) {
      try {
        std::string hex = m[1].str();
        int r = std::stoi(hex.substr(0, 2), nullptr, 16);
        int g = std::stoi(hex.substr(2, 2), nullptr, 16);
        int b = std::stoi(hex.substr(4, 2), nullptr, 16);
        return RGB{r, g, b};
      } catch (...) {
        return std::unexpected(TerminalColorError::ParseError);
      }
    }
    return std::unexpected(TerminalColorError::ParseError);
  }

  static Result queryPaletteIndexPosix(int index) noexcept {
    if (!index_valid(index))
      return std::unexpected(TerminalColorError::Unsupported);
    std::string osc = make_osc_query("4;" + std::to_string(index) + ";?");
    auto replyOrErr = send_osc_and_read(osc);
    if (!replyOrErr)
      return std::unexpected(replyOrErr.error());
    auto rgbOrErr = parse_color_from_reply(*replyOrErr);
    if (!rgbOrErr)
      return std::unexpected(rgbOrErr.error());
    return *rgbOrErr;
  }

  static Result queryForegroundPosix() noexcept {
    std::string osc = make_osc_query("10;?");
    auto replyOrErr = send_osc_and_read(osc);
    if (!replyOrErr)
      return std::unexpected(replyOrErr.error());
    auto rgbOrErr = parse_color_from_reply(*replyOrErr);
    if (!rgbOrErr)
      return std::unexpected(rgbOrErr.error());
    return *rgbOrErr;
  }

  static Result queryBackgroundPosix() noexcept {
    std::string osc = make_osc_query("11;?");
    auto replyOrErr = send_osc_and_read(osc);
    if (!replyOrErr)
      return std::unexpected(replyOrErr.error());
    auto rgbOrErr = parse_color_from_reply(*replyOrErr);
    if (!rgbOrErr)
      return std::unexpected(rgbOrErr.error());
    return *rgbOrErr;
  }

#endif // POSIX

}; // class TerminalColorQuery