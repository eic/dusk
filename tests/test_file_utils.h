#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <random>
#include <string>
#include <system_error>

namespace testutils {
namespace fs = std::filesystem;

inline fs::path uniqueTempPath(const std::string& prefix, const std::string& extension) {
  static std::atomic<std::uint64_t> counter{0};
  static std::random_device rd;

  const auto now     = std::chrono::steady_clock::now().time_since_epoch().count();
  const auto suffix  = counter.fetch_add(1, std::memory_order_relaxed);
  const auto entropy = (static_cast<std::uint64_t>(rd()) << 32) ^ static_cast<std::uint64_t>(rd());

  return fs::temp_directory_path() /
         (prefix + "_" + std::to_string(now) + "_" + std::to_string(suffix) + "_" +
          std::to_string(entropy) + extension);
}

inline void removeNoThrow(const fs::path& path) {
  std::error_code ec;
  fs::remove(path, ec);
}
} // namespace testutils
