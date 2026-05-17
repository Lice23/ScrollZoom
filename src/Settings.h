#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ScrollZoom
{
struct Settings
{
    float stepSize{0.5f};
    float minZoomRatio{0.5f};
    std::int32_t startZoom{0};
    float minFovMult{3.0f};
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
