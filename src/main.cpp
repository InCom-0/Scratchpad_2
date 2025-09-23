#include <console_colors.hpp>

#include <iostream>

int main() {
  auto fg = TerminalColorQuery::queryForeground();
  if (fg) {
    auto [r, g, b] = fg.value();
    std::cerr << "Foreground RGB: " << r << "," << g << "," << b << "\n";
  } else {
    std::cerr << "Could not query FG: "
              << TerminalColorQuery::to_string(fg.error()) << "\n";
    // fallback logic here (e.g. assume defaults or use other config)
  }

  // Query palette index 1 (ANSI red)

  for (size_t id = 0; id < 16; ++id) {
    auto idx = TerminalColorQuery::queryPaletteIndex(id);
    if (idx) {
      auto [r, g, b] = idx.value();
      std::cerr << "Index " << id << ": " << r << "," << g << "," << b << "\n";
    } else {
      std::cerr << "Palette query failed: "
                << TerminalColorQuery::to_string(idx.error()) << "\n";
    }
  }
}