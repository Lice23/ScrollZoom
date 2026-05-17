#include "TullFrameworkCompat.h"
#include "ZoomController.h"

#include <Windows.h>

namespace
{
enum class TullAimMode : std::int8_t
{
    iNormal = 0,
    iSide = 1
};

struct TullSwitchAimEvent
{
    TullAimMode mode;
};
static_assert(sizeof(TullSwitchAimEvent) == 0x1);

using GetSwitchAimSource_t = RE::BSTEventSource<TullSwitchAimEvent> *(*)();
using IsSideAimActive_t = bool (*)();

class TullSwitchAimSink final : public RE::BSTEventSink<TullSwitchAimEvent>
{
  public:
    RE::BSEventNotifyControl ProcessEvent(const TullSwitchAimEvent &a_event,
                                          RE::BSTEventSource<TullSwitchAimEvent> *) override
    {
        const auto mode =
            a_event.mode == TullAimMode::iSide ? ScrollZoom::AimMode::kSide : ScrollZoom::AimMode::kNormal;
        ScrollZoom::RebaseZoomForAimMode(mode);
        return RE::BSEventNotifyControl::kContinue;
    }
};

TullSwitchAimSink g_tullSwitchAimSink;
bool g_tullFrameworkCompatInitialized = false;
} // namespace

namespace ScrollZoom
{
void InitializeTullFrameworkCompat()
{
    if (g_tullFrameworkCompatInitialized)
    {
        return;
    }

    auto *module = GetModuleHandleA("TullFramework.dll");
    if (!module)
    {
        logger::debug("TullFramework compatibility disabled: module not loaded");
        return;
    }

    auto *getSwitchAimSource = reinterpret_cast<GetSwitchAimSource_t>(GetProcAddress(module, "GetSwitchAimSource"));
    auto *isSideAimActive = reinterpret_cast<IsSideAimActive_t>(GetProcAddress(module, "IsSideAimActive"));
    if (!getSwitchAimSource || !isSideAimActive)
    {
        logger::warn("TullFramework compatibility disabled: required exports not found");
        return;
    }

    SetCurrentAimMode(isSideAimActive() ? AimMode::kSide : AimMode::kNormal);

    auto *source = getSwitchAimSource();
    if (!source)
    {
        logger::debug("TullFramework compatibility disabled: switch aim event source unavailable");
        return;
    }

    source->RegisterSink(&g_tullSwitchAimSink);
    g_tullFrameworkCompatInitialized = true;

    logger::info("TullFramework compatibility enabled");
}
} // namespace ScrollZoom
