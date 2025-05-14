#include <hyprgraphics/color/Color.hpp>
#include <hyprland/src/helpers/memory/Memory.hpp>
#include <hyprland/src/plugins/HookSystem.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprlang.hpp>
#include <optional>
#include <string>
#include <unistd.h>

#define private public
#include <hyprland/src/config/ConfigManager.hpp>
#undef private

#include "desktop/WindowRule.hpp"
#include "globals.hpp"

inline std::optional<std::string> selected;

/* called on focus change with new window */
void focusChanged(PHLWINDOW window) {
    auto last = selected;
    bool set = false;

    for (auto rule : window->m_matchedRules) {
        if (rule->m_ruleType == CWindowRule::RULE_PLUGIN && rule->m_rule.starts_with("plugin:device")) {
            auto device = CVarList(rule->m_rule, 0, ' ')[1];

            Debug::log(LOG, "[device-windowrule] setting device to {}", device);

            if (device.empty()) selected = {};
            else selected = device;

            set = true;
            break;
        }
    }

    // unset if no window rule
    if (!set) selected = {};

    if (last != selected) {
        Debug::log(LOG, "[device-windowrule] re-setting input device settings");

        // see HyprCtl.cpp line 1119
        g_pInputManager->setKeyboardLayout();     // update kb layout
        g_pInputManager->setPointerConfigs();     // update mouse cfgs
        g_pInputManager->setTouchDeviceConfigs(); // update touch device cfgs
        g_pInputManager->setTabletConfigs();      // update tablets
    }
}

typedef Hyprlang::CConfigValue* (*origGetConfigValueSafeDevice)(void*, const std::string& dev, const std::string& val, const std::string& fallback);
inline CFunctionHook* g_pGetConfigValueSafeDeviceHook = nullptr;
Hyprlang::CConfigValue* hkGetConfigValueSafeDevice(void* thisptr, const std::string& dev, const std::string& val, const std::string& fallback) {
    if (selected.has_value()) {
        const auto VAL = g_pConfigManager->m_config->getSpecialConfigValuePtr("device", val.c_str(), selected->c_str());

        if (VAL && VAL->m_bSetByUser)
            return VAL;
    }

    // fall back to normal config if not set
    return (*(origGetConfigValueSafeDevice)g_pGetConfigValueSafeDeviceHook->m_original)(thisptr, dev, val, fallback);
}

/* hooks a function hook */
CFunctionHook* hook(std::string name, std::string object, void* function) {
    auto names = HyprlandAPI::findFunctionsByName(PHANDLE, name);

    // we hook on member functions, so search for them
    for (auto match : names) {
        if (!match.demangled.starts_with(object)) continue;

        Debug::log(LOG, "[device-windowrule] hooking on {} for {}::{}", match.demangled, object, name);

        auto hook = HyprlandAPI::createFunctionHook(PHANDLE, match.address, function);
        hook->hook();

        return hook;
    }

    Debug::log(ERR, "Could not find hooking candidate for {}::{}", object, name);
    throw std::runtime_error("no hook candidate found");
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    // check that header version aligns with running version
    const std::string HASH = __hyprland_api_get_hash();
    if (false && HASH != GIT_COMMIT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, "[device-windowrule] Failed to load, mismatched headers!", CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        HyprlandAPI::addNotification(PHANDLE, std::format("[device-windowrule] Built with: {}, running: {}", GIT_COMMIT_HASH, HASH), CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("version mismatch");
    }

    // try hooking
    try {
        g_pGetConfigValueSafeDeviceHook = hook("getConfigValueSafeDevice", "CConfigManager", (void*) &hkGetConfigValueSafeDevice);
    } catch (...) {
        HyprlandAPI::addNotification(PHANDLE, "[device-windowrule] Failed to load, hooks could not be made!", CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("hooks failed");
    }

    static const auto FOCUS_CALLBACK = HyprlandAPI::registerCallbackDynamic(PHANDLE, "activeWindow", [&](void* self, SCallbackInfo&, std::any data) {
        focusChanged(std::any_cast<PHLWINDOW>(data));
    });

    return {"device-windowrule", "a plugin to apply input device config based on the focused window", "Virt", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() { }

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}
