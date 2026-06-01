#include "Settings.h"

#include <charconv>
#include <string>
#include <system_error>

namespace ScrollZoom
{
Settings *Settings::GetSingleton()
{
    static Settings singleton;
    return &singleton;
}

void Settings::Load()
{
    constexpr auto path = L"Data/F4SE/Plugins/ScrollZoom.ini";

    CSimpleIniA ini;
    ini.SetUnicode();

    if (ini.LoadFile(path) < 0)
    {
        logger::info("No INI file found, using defaults");
        return;
    }

    stepSize = static_cast<float>(ini.GetDoubleValue("General", "fStepSize", 0.5));
    minZoomRatio = static_cast<float>(ini.GetDoubleValue("General", "fMinZoomRatio", 0.5));
    maxZoomRatio = static_cast<float>(ini.GetDoubleValue("General", "fMaxZoomRatio", 1.0));
    startZoom = static_cast<std::int32_t>(ini.GetLongValue("General", "iStartZoom", 0));
    minFovMult = static_cast<float>(ini.GetDoubleValue("General", "fMinFovMult", 3.0));
    debugLogging = ini.GetBoolValue("General", "bDebugLogging", false);

    if (maxZoomRatio < minZoomRatio)
    {
        logger::warn("fMaxZoomRatio ({:.2f}) is below fMinZoomRatio ({:.2f}); clamping to fMinZoomRatio", maxZoomRatio,
                     minZoomRatio);
        maxZoomRatio = minZoomRatio;
    }

    const char *rawOMODs = ini.GetValue("General", "sExcludedOMODs", "");
    ParseExcludedOMODs(rawOMODs);

    logger::info("Settings loaded: stepSize={:.2f}, minZoomRatio={:.2f}, maxZoomRatio={:.2f}, startZoom={}, "
                 "minFovMult={:.2f}, debugLogging={}",
                 stepSize, minZoomRatio, maxZoomRatio, startZoom, minFovMult, debugLogging);
}

void Settings::ResolveExcludedOMODs()
{
    auto *dataHandler = RE::TESDataHandler::GetSingleton();
    if (!dataHandler)
    {
        return;
    }

    for (auto &[plugin, rawID] : pendingOMODs)
    {
        auto formID = dataHandler->LookupFormID(rawID, plugin);
        if (formID != 0)
        {
            excludedOMODs.insert(formID);
            logger::debug("Excluded OMOD resolved: {}:{:06X} -> {:08X}", plugin, rawID, formID);
        }
        else
        {
            logger::warn("Failed to resolve OMOD: {}:{:06X}", plugin, rawID);
        }
    }

    pendingOMODs.clear();

    if (!excludedOMODs.empty())
    {
        logger::info("Total excluded OMODs: {}", excludedOMODs.size());
    }
}

void Settings::ParseExcludedOMODs(const char *a_raw)
{
    if (!a_raw || a_raw[0] == '\0')
    {
        return;
    }

    std::string raw(a_raw);
    std::size_t pos = 0;

    while (pos < raw.size())
    {
        auto commaPos = raw.find(',', pos);
        std::string token = raw.substr(pos, commaPos == std::string::npos ? std::string::npos : commaPos - pos);
        pos = commaPos == std::string::npos ? raw.size() : commaPos + 1;

        auto start = token.find_first_not_of(" \t");
        auto end = token.find_last_not_of(" \t");
        if (start == std::string::npos)
        {
            continue;
        }
        token = token.substr(start, end - start + 1);

        auto colonPos = token.find(':');
        if (colonPos == std::string::npos || colonPos == 0 || colonPos + 1 >= token.size())
        {
            logger::warn("Invalid OMOD entry (expected Plugin:FormID): {}", token);
            continue;
        }

        std::string plugin = token.substr(0, colonPos);
        std::string formStr = token.substr(colonPos + 1);

        if (formStr.size() > 2 && formStr[0] == '0' && (formStr[1] == 'x' || formStr[1] == 'X'))
        {
            formStr = formStr.substr(2);
        }

        std::uint32_t rawID = 0;
        auto [ptr, ec] = std::from_chars(formStr.data(), formStr.data() + formStr.size(), rawID, 16);
        if (ec != std::errc{} || ptr != formStr.data() + formStr.size())
        {
            logger::warn("Invalid hex FormID: {}", formStr);
            continue;
        }

        pendingOMODs.emplace_back(plugin, rawID);
    }
}
} // namespace ScrollZoom
