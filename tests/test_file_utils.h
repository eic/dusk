#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>

namespace testutils {
namespace fs = std::filesystem;

inline fs::path uniqueTempPath(const std::string& prefix, const std::string& extension) {
  static std::atomic<std::uint64_t> counter{0};
  const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const auto suffix = counter.fetch_add(1, std::memory_order_relaxed);
  return fs::temp_directory_path() /
         (prefix + "_" + std::to_string(now) + "_" + std::to_string(suffix) + extension);
}

inline void removeNoThrow(const fs::path& path) {
  std::error_code ec;
  fs::remove(path, ec);
}
}  // namespace testutils
