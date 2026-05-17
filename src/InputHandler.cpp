#include "InputHandler.h"
#include "ZoomController.h"

namespace ScrollZoom
{
inline constexpr std::int32_t kMouseWheelUp = 0x800;
inline constexpr std::int32_t kMouseWheelDown = 0x900;

class ScrollZoomHandler : public RE::PlayerInputHandler
{
  public:
    explicit ScrollZoomHandler(RE::PlayerControlsData &a_data) : PlayerInputHandler(a_data)
    {
        inputEventHandlingEnabled = true;
    }

    bool ShouldHandleEvent(const RE::InputEvent *a_event) override
    {
        return a_event && *a_event->eventType == RE::INPUT_EVENT_TYPE::kButton;
    }

    void OnButtonEvent(const RE::ButtonEvent *a_event) override
    {
        if (!a_event || !a_event->QJustPressed())
        {
            return;
        }

        if (*a_event->device != RE::INPUT_DEVICE::kMouse)
        {
            return;
        }

        std::int32_t id = a_event->QIDCode();
        if (id != kMouseWheelUp && id != kMouseWheelDown)
        {
            return;
        }

        auto *player = RE::PlayerCharacter::GetSingleton();
        if (!player)
        {
            return;
        }

        auto gs = player->gunState;
        if (gs != RE::GUN_STATE::kSighted && gs != RE::GUN_STATE::kFireSighted)
        {
            return;
        }

        if (!g_zoomState.isActive)
        {
            OnIronSightsEnter();
            if (!g_zoomState.isActive)
            {
                return;
            }
        }

        OnScrollWheel(id == kMouseWheelDown);
    }

    void PerFrameUpdate() override
    {
        auto *player = RE::PlayerCharacter::GetSingleton();
        bool inIronSights =
            player && (player->gunState == RE::GUN_STATE::kSighted || player->gunState == RE::GUN_STATE::kFireSighted);

        if (inIronSights && !g_zoomState.isActive)
        {
            OnIronSightsEnter();
        }
        else if (!inIronSights && g_zoomState.isActive)
        {
            OnIronSightsExit();
        }
    }
};

void InstallInputHandler()
{
    auto *playerControls = RE::PlayerControls::GetSingleton();
    if (!playerControls)
    {
        logger::error("Failed to get PlayerControls singleton");
        return;
    }

    auto *handler = new ScrollZoomHandler(playerControls->data);
    playerControls->RegisterHandler(handler);

    logger::info("ScrollZoom input handler registered");
}
} // namespace ScrollZoom
