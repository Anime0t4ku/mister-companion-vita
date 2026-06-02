#pragma once

#include <string>
#include <vector>

namespace WallpaperDb {

struct Entry {
    std::string name;
    std::string url;
};

bool fetchEntries(const std::string& dbUrl,
                  const std::string& rawBase,
                  const std::string& filterMode,
                  std::vector<Entry>& entries,
                  std::string& error);

bool parseEntriesFromData(const std::vector<unsigned char>& data,
                          const std::string& rawBase,
                          const std::string& filterMode,
                          std::vector<Entry>& entries,
                          std::string& error);

} // namespace WallpaperDb
