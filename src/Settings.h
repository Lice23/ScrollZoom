#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ScrollZoom
{

// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
inline constexpr std::int32_t kMouseMiddleButton = 0x02;
  
struct Settings
{
    float stepSize{0.5f};
    float minZoomRatio{0.5f};
    float maxZoomRatio{1.0f};
    std::int32_t startZoom{0};
    float minFovMult{3.0f};
    std::int32_t toggleKey{kMouseMiddleButton};
    bool debugLogging{false};

    std::unordered_set<std::uint32_t> excludedOMODs;

    static Settings *GetSingleton();
    void Load();
    void ResolveExcludedOMODs();

  private:
    std::vector<std::pair<std::string, std::uint32_t>> pendingOMODs;

    void ParseExcludedOMODs(const char *a_raw);
};
} // namespace ScrollZoom
