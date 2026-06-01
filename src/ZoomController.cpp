#include "ZoomController.h"
#include "Settings.h"

#include <algorithm>
#include <vector>

namespace ScrollZoom
{
namespace
{
constexpr std::uint32_t kFNVOffset = 2166136261u;
constexpr std::uint32_t kFNVPrime = 16777619u;

[[nodiscard]] const char *GetAimModeName(AimMode a_mode)
{
    return a_mode == AimMode::kSide ? "Side" : "Normal";
}

[[nodiscard]] bool IsValidAimMode(std::uint32_t a_mode)
{
    return a_mode == static_cast<std::uint32_t>(AimMode::kNormal) ||
           a_mode == static_cast<std::uint32_t>(AimMode::kSide);
}

[[nodiscard]] ZoomKey MakeZoomKey(const WeaponZoomInfo &a_info)
{
    return {a_info.weaponFormID, static_cast<std::uint8_t>(g_currentAimMode), a_info.scopeOMODSignature};
}

[[nodiscard]] bool IsScopeZoomOMOD(const RE::BGSMod::Attachment::Mod *a_mod)
{
    if (!a_mod)
    {
        return false;
    }

    constexpr auto hasScope = static_cast<std::uint32_t>(RE::BGSMod::Property::WEAPON_PROPERTY::kHasScope);
    constexpr auto zoomFovMult = static_cast<std::uint32_t>(RE::BGSMod::Property::WEAPON_PROPERTY::kZoomDataFOVMult);
    constexpr auto zoomData = static_cast<std::uint32_t>(RE::BGSMod::Property::WEAPON_PROPERTY::kZoomData);

    for (const auto &property : a_mod->GetBuffer<RE::BGSMod::Property::Mod>(1))
    {
        const auto target = static_cast<std::uint32_t>(property.target);
        if (target == hasScope || target == zoomFovMult || target == zoomData)
        {
            return true;
        }
    }

    return false;
}

[[nodiscard]] bool TryGetSavedZoom(const ZoomKey &a_key, float &a_fovMult, bool &a_legacyFallback)
{
    auto it = g_savedZoomLevels.find(a_key);
    if (it != g_savedZoomLevels.end())
    {
        a_fovMult = it->second;
        a_legacyFallback = false;
        return true;
    }

    if (a_key.aimMode == static_cast<std::uint8_t>(AimMode::kNormal) && a_key.scopeOMODSignature != 0)
    {
        ZoomKey legacyKey{a_key.weaponFormID, static_cast<std::uint8_t>(AimMode::kNormal), 0};
        it = g_savedZoomLevels.find(legacyKey);
        if (it != g_savedZoomLevels.end())
        {
            a_fovMult = it->second;
            a_legacyFallback = true;
            return true;
        }
    }

    return false;
}

[[nodiscard]] std::uint32_t BuildScopeOMODSignature(RE::PlayerCharacter *a_player, RE::TESForm *a_baseObject)
{
    if (!a_player || !a_player->inventoryList || !a_baseObject)
    {
        return 0;
    }

    std::vector<std::uint32_t> scopeOMODs;
    RE::BSAutoReadLock lock{a_player->inventoryList->rwLock};
    a_player->inventoryList->ForEachStack(
        [a_baseObject](RE::BGSInventoryItem &item) { return item.object == a_baseObject; },
        [&](RE::BGSInventoryItem &, RE::BGSInventoryItem::Stack &stack) {
            if (!stack.IsEquipped())
            {
                return true;
            }

            if (!stack.extra)
            {
                return false;
            }

            auto *xInst = stack.extra->GetByType<RE::BGSObjectInstanceExtra>();
            if (!xInst)
            {
                return false;
            }

            for (const auto &idx : xInst->GetIndexData())
            {
                auto *omod = RE::TESForm::GetFormByID<RE::BGSMod::Attachment::Mod>(idx.objectID);
                if (IsScopeZoomOMOD(omod))
                {
                    scopeOMODs.push_back(idx.objectID);
                }
            }

            return false;
        });

    if (scopeOMODs.empty())
    {
        return 0;
    }

    std::sort(scopeOMODs.begin(), scopeOMODs.end());
    scopeOMODs.erase(std::unique(scopeOMODs.begin(), scopeOMODs.end()), scopeOMODs.end());

    std::uint32_t signature = kFNVOffset;
    for (const auto omodID : scopeOMODs)
    {
        signature ^= omodID;
        signature *= kFNVPrime;
    }

    return signature;
}

void SaveActiveZoomLevel()
{
    if (g_zoomState.isActive && g_zoomState.activeKey.weaponFormID)
    {
        g_savedZoomLevels[g_zoomState.activeKey] = g_zoomState.currentFovMult;
    }
}

void ClearActiveZoomState(bool a_restoreOriginal)
{
    if (a_restoreOriginal && g_zoomState.cachedZoomData)
    {
        g_zoomState.cachedZoomData->zoomData.fovMult = g_zoomState.originalFovMult;
    }

    g_zoomState.Reset();
}
} // namespace

bool HasExcludedOMOD()
{
    auto *settings = Settings::GetSingleton();
    if (settings->excludedOMODs.empty())
    {
        return false;
    }

    auto *player = RE::PlayerCharacter::GetSingleton();
    if (!player || !player->inventoryList)
    {
        return false;
    }

    if (!player->currentProcess || !player->currentProcess->middleHigh)
    {
        return false;
    }

    auto &equippedItems = player->currentProcess->middleHigh->equippedItems;
    if (equippedItems.empty())
    {
        return false;
    }

    auto *baseObject = equippedItems[0].item.object;
    if (!baseObject)
    {
        return false;
    }

    bool excluded = false;
    RE::BSAutoReadLock lock{player->inventoryList->rwLock};
    player->inventoryList->ForEachStack([baseObject](RE::BGSInventoryItem &item) { return item.object == baseObject; },
                                        [&](RE::BGSInventoryItem &, RE::BGSInventoryItem::Stack &stack) {
                                            if (!stack.IsEquipped() || !stack.extra)
                                            {
                                                return true;
                                            }

                                            auto *xInst = stack.extra->GetByType<RE::BGSObjectInstanceExtra>();
                                            if (!xInst)
                                            {
                                                return false;
                                            }

                                            for (const auto &idx : xInst->GetIndexData())
                                            {
                                                if (settings->excludedOMODs.contains(idx.objectID))
                                                {
                                                    excluded = true;
                                                    return false;
                                                }
                                            }

                                            return false;
                                        });

    return excluded;
}

WeaponZoomInfo GetEquippedWeaponZoomInfo()
{
    auto *player = RE::PlayerCharacter::GetSingleton();
    if (!player || !player->currentProcess || !player->currentProcess->middleHigh)
    {
        return {};
    }

    auto &equippedItems = player->currentProcess->middleHigh->equippedItems;
    if (equippedItems.empty())
    {
        return {};
    }

    auto &item = equippedItems[0];
    if (!item.item.object || !item.item.instanceData)
    {
        return {};
    }

    auto *instance = static_cast<RE::TESObjectWEAP::InstanceData *>(item.item.instanceData.get());
    if (!instance || !instance->zoomData)
    {
        return {};
    }

    const auto scopeOMODSignature = BuildScopeOMODSignature(player, item.item.object);
    return {instance->zoomData, item.item.object->GetFormID(), scopeOMODSignature};
}

bool IsPlayerInIronSights()
{
    auto *player = RE::PlayerCharacter::GetSingleton();
    return player && (player->gunState == RE::GUN_STATE::kSighted || player->gunState == RE::GUN_STATE::kFireSighted);
}

void SetCurrentAimMode(AimMode a_mode)
{
    g_currentAimMode = a_mode;
}

void RebaseZoomForAimMode(AimMode a_mode)
{
    if (g_currentAimMode == a_mode && !g_zoomState.isActive)
    {
        return;
    }

    SaveActiveZoomLevel();
    ClearActiveZoomState(false);
    SetCurrentAimMode(a_mode);

    logger::debug("TullFramework aim mode changed: {}", GetAimModeName(a_mode));

    if (IsPlayerInIronSights())
    {
        OnIronSightsEnter();
    }
}

void OnIronSightsEnter()
{
    auto info = GetEquippedWeaponZoomInfo();
    if (!info.zoomData)
    {
        return;
    }

    auto *settings = Settings::GetSingleton();

    if (HasExcludedOMOD())
    {
        return;
    }

    float fovMult = info.zoomData->zoomData.fovMult;
    if (fovMult < settings->minFovMult)
    {
        return;
    }

    const auto key = MakeZoomKey(info);
    g_zoomState.originalFovMult = fovMult;
    g_zoomState.cachedZoomData = info.zoomData;
    g_zoomState.activeKey = key;
    g_zoomState.isActive = true;

    float minFov = fovMult * settings->minZoomRatio;
    float maxFov = fovMult * settings->maxZoomRatio;
    float savedFovMult = 0.0f;
    bool legacyFallback = false;
    if (TryGetSavedZoom(key, savedFovMult, legacyFallback))
    {
        g_zoomState.currentFovMult = std::clamp(savedFovMult, minFov, maxFov);
        logger::debug("OnIronSightsEnter: restored saved={:.3f}, clamped={:.3f}, original={:.3f}, minFov={:.3f}, "
                      "maxFov={:.3f}, weaponFormID={:08X}, aimMode={}, scopeSig={:08X}, legacyFallback={}",
                      savedFovMult, g_zoomState.currentFovMult, fovMult, minFov, maxFov, key.weaponFormID,
                      GetAimModeName(g_currentAimMode), key.scopeOMODSignature, legacyFallback);
    }
    else
    {
        switch (settings->startZoom)
        {
        case 1:
            g_zoomState.currentFovMult = (maxFov + minFov) * 0.5f;
            break;
        case 2:
            g_zoomState.currentFovMult = maxFov;
            break;
        default:
            g_zoomState.currentFovMult = minFov;
            break;
        }
        logger::debug("OnIronSightsEnter: no saved, start={:.3f}, original={:.3f}, minFov={:.3f}, maxFov={:.3f}, "
                      "weaponFormID={:08X}, aimMode={}, scopeSig={:08X}",
                      g_zoomState.currentFovMult, fovMult, minFov, maxFov, key.weaponFormID,
                      GetAimModeName(g_currentAimMode), key.scopeOMODSignature);
    }

    info.zoomData->zoomData.fovMult = g_zoomState.currentFovMult;
}

void OnIronSightsExit()
{
    if (!g_zoomState.isActive)
    {
        return;
    }

    logger::debug("OnIronSightsExit: saving={:.3f}, restoring={:.3f}, weaponFormID={:08X}, aimMode={}, scopeSig={:08X}",
                  g_zoomState.currentFovMult, g_zoomState.originalFovMult, g_zoomState.activeKey.weaponFormID,
                  GetAimModeName(static_cast<AimMode>(g_zoomState.activeKey.aimMode)),
                  g_zoomState.activeKey.scopeOMODSignature);

    SaveActiveZoomLevel();
    ClearActiveZoomState(true);
}

void OnScrollWheel(bool a_zoomOut)
{
    if (!g_zoomState.isActive || !g_zoomState.cachedZoomData)
    {
        return;
    }

    auto *settings = Settings::GetSingleton();

    if (a_zoomOut)
    {
        float minFovMult = g_zoomState.originalFovMult * settings->minZoomRatio;
        g_zoomState.currentFovMult = (std::max)(g_zoomState.currentFovMult - settings->stepSize, minFovMult);
    }
    else
    {
        float maxFovMult = g_zoomState.originalFovMult * settings->maxZoomRatio;
        g_zoomState.currentFovMult = (std::min)(g_zoomState.currentFovMult + settings->stepSize, maxFovMult);
    }

    g_zoomState.cachedZoomData->zoomData.fovMult = g_zoomState.currentFovMult;
    logger::debug("Zoom: fovMult={:.3f} ({}x)", g_zoomState.currentFovMult, 1.0f / g_zoomState.currentFovMult);
}

void ResetZoomState()
{
    ClearActiveZoomState(true);
}

void F4SEAPI SaveCallback(const F4SE::SerializationInterface *a_intfc)
{
    SaveActiveZoomLevel();

    auto count = static_cast<std::uint32_t>(g_savedZoomLevels.size());
    if (!a_intfc->OpenRecord(kRecordType, kCoSaveVersion))
    {
        logger::warn("Co-Save: failed to open record");
        return;
    }

    a_intfc->WriteRecordData(&count, sizeof(count));
    for (auto &[key, fovMult] : g_savedZoomLevels)
    {
        const auto aimMode = static_cast<std::uint32_t>(key.aimMode);
        a_intfc->WriteRecordData(&key.weaponFormID, sizeof(key.weaponFormID));
        a_intfc->WriteRecordData(&aimMode, sizeof(aimMode));
        a_intfc->WriteRecordData(&key.scopeOMODSignature, sizeof(key.scopeOMODSignature));
        a_intfc->WriteRecordData(&fovMult, sizeof(fovMult));
    }

    logger::debug("Co-Save: saved {} entries", count);
}

void F4SEAPI LoadCallback(const F4SE::SerializationInterface *a_intfc)
{
    std::uint32_t type, version, length;
    while (a_intfc->GetNextRecordInfo(type, version, length))
    {
        if (type != kRecordType)
        {
            continue;
        }

        if (version != 1 && version != kCoSaveVersion)
        {
            logger::warn("Co-Save: version mismatch (got {}, expected 1 or {})", version, kCoSaveVersion);
            continue;
        }

        std::uint32_t count = 0;
        a_intfc->ReadRecordData(&count, sizeof(count));

        for (std::uint32_t i = 0; i < count; ++i)
        {
            std::uint32_t formID = 0;
            std::uint32_t aimMode = static_cast<std::uint32_t>(AimMode::kNormal);
            std::uint32_t scopeOMODSignature = 0;
            float fovMult = 0.0f;

            a_intfc->ReadRecordData(&formID, sizeof(formID));
            if (version == kCoSaveVersion)
            {
                a_intfc->ReadRecordData(&aimMode, sizeof(aimMode));
                a_intfc->ReadRecordData(&scopeOMODSignature, sizeof(scopeOMODSignature));
            }
            a_intfc->ReadRecordData(&fovMult, sizeof(fovMult));

            auto resolved = a_intfc->ResolveFormID(formID);
            if (resolved && IsValidAimMode(aimMode))
            {
                ZoomKey key{*resolved, static_cast<std::uint8_t>(aimMode), scopeOMODSignature};
                g_savedZoomLevels[key] = fovMult;
            }
        }

        logger::debug("Co-Save: loaded {} entries ({} resolved)", count, g_savedZoomLevels.size());
    }
}

void F4SEAPI RevertCallback(const F4SE::SerializationInterface *)
{
    ResetZoomState();
    g_savedZoomLevels.clear();
}
} // namespace ScrollZoom
