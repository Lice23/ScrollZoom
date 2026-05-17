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

struct WeaponZoomInfo
{
    RE::BGSZoomData *zoomData{nullptr};
    std::uint32_t weaponFormID{0};
    std::uint32_t scopeOMODSignature{0};
};

struct ZoomState
{
    float originalFovMult{0.0f};
    float currentFovMult{1.0f};
    RE::BGSZoomData *cachedZoomData{nullptr};
    ZoomKey activeKey{};
    bool isActive{false};

    void Reset()
    {
        originalFovMult = 0.0f;
        currentFovMult = 1.0f;
        cachedZoomData = nullptr;
        activeKey = {};
        isActive = false;
    }
};

inline ZoomState g_zoomState;
inline AimMode g_currentAimMode{AimMode::kNormal};
inline std::unordered_map<ZoomKey, float, ZoomKeyHash> g_savedZoomLevels;

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
