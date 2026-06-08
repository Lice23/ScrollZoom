#pragma once

#include <cstdint>
#include <unordered_map>

namespace RE
{
class BGSZoomData;
}

namespace F4SE
{
class SerializationInterface;
}

namespace ScrollZoom
{
inline constexpr std::uint32_t kCoSaveID = 'SCZM';
inline constexpr std::uint32_t kCoSaveVersion = 2;
inline constexpr std::uint32_t kRecordType = 'ZOOM';

enum class AimMode : std::uint8_t
{
    kNormal = 0,
    kSide = 1
};

struct ZoomKey
{
    std::uint32_t weaponFormID{0};
    std::uint8_t aimMode{0};
    std::uint32_t scopeOMODSignature{0};

    [[nodiscard]] bool operator==(const ZoomKey &a_rhs) const noexcept
    {
        return weaponFormID == a_rhs.weaponFormID && aimMode == a_rhs.aimMode &&
               scopeOMODSignature == a_rhs.scopeOMODSignature;
    }
};

struct ZoomKeyHash
{
    [[nodiscard]] std::size_t operator()(const ZoomKey &a_key) const noexcept
    {
        std::size_t seed = std::hash<std::uint32_t>{}(a_key.weaponFormID);
        seed ^= std::hash<std::uint8_t>{}(a_key.aimMode) + 0x9E3779B9u + (seed << 6) + (seed >> 2);
        seed ^= std::hash<std::uint32_t>{}(a_key.scopeOMODSignature) + 0x9E3779B9u + (seed << 6) + (seed >> 2);
        return seed;
    }
};

struct ScopeParams
{
    float init{-1.0f};
    float min{-1.0f};
    float max{-1.0f};
    float step{-1.0f};
    bool isValid{false};

    ScopeParams() {}

    ScopeParams(float initP, float minP, float maxP, float stepP) {
        init = initP != -1.0f ? initP : minP; // If init is not defined, default to min as initZoom
        min = minP;
        max = maxP;
        step = stepP; 
        isValid = ((initP != -1.0f) && (minP != -1.0f) && (maxP != -1.0f) && (stepP != -1.0f));
    }
};

struct WeaponZoomInfo
{
    RE::BGSZoomData *zoomData{nullptr};
    std::uint32_t weaponFormID{0};
    std::uint32_t scopeOMODSignature{0};
    ScopeParams scopeParams;
};

struct ZoomState
{
    float originalFovMult{1.0f};
    float currentFovMult{1.0f};
    ScopeParams scopeParams;
    RE::BGSZoomData *cachedZoomData{nullptr};
    ZoomKey activeKey{};
    bool isActive{false};

    void Reset()
    {
        originalFovMult = 1.0f;
        currentFovMult = 1.0f;
        scopeParams = ScopeParams();
        cachedZoomData = nullptr;
        activeKey = {};
        isActive = false;
    }
};

inline ZoomState g_zoomState;
inline AimMode g_currentAimMode{AimMode::kNormal};
inline std::unordered_map<ZoomKey, float, ZoomKeyHash> g_savedZoomLevels;

const std::string ZOOM_KW_INIT = "dn_ScrollZoom_Init";
const std::string ZOOM_KW_MAX = "dn_ScrollZoom_Max";
const std::string ZOOM_KW_MIN = "dn_ScrollZoom_Min";
const std::string ZOOM_KW_STEP = "dn_ScrollZoom_Step";
const std::string ZOOM_KW_FIXED = "dn_ScrollZoom_Fixed";

float getFloatFromKeyword(const std::string& str, const std::string& kw);
WeaponZoomInfo GetEquippedWeaponZoomInfo();
bool HasExcludedOMOD();
bool IsPlayerInIronSights();
void SetCurrentAimMode(AimMode a_mode);
void RebaseZoomForAimMode(AimMode a_mode);
void OnIronSightsEnter();
void OnIronSightsExit();
void OnScrollWheel(bool a_zoomOut);
void ResetZoomState();

void F4SEAPI SaveCallback(const F4SE::SerializationInterface *a_intfc);
void F4SEAPI LoadCallback(const F4SE::SerializationInterface *a_intfc);
void F4SEAPI RevertCallback(const F4SE::SerializationInterface *a_intfc);
} // namespace ScrollZoom
