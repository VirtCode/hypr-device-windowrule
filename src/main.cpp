#include <hyprgraphics/color/Color.hpp>
#include <hyprland/src/helpers/memory/Memory.hpp>
#include <hyprland/src/plugins/HookSystem.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/devices/IKeyboard.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprlang.hpp>
#include <string>
#include <unistd.h>

#define private public
#include <hyprland/src/config/ConfigManager.hpp>
#undef private

#include "globals.hpp"
#include "rules.hpp"

typedef Hyprlang::CConfigValue* (*origGetConfigValueSafeDevice)(void*, const std::string& dev, const std::string& val, const std::string& fallback);
inline CFunctionHook* g_pGetConfigValueSafeDeviceHook = nullptr;
Hyprlang::CConfigValue* hkGetConfigValueSafeDevice(void* thisptr, const std::string& dev, const std::string& val, const std::string& fallback) {
    const auto value = g_pDeviceWindowrules->getConfig(dev, val);
    if (value) return value;

    // fall back to normal config if not set
    const auto orig = (*(origGetConfigValueSafeDevice)g_pGetConfigValueSafeDeviceHook->m_original)(thisptr, dev, val, fallback);
    static Hyprlang::CConfigValue true_value = Hyprlang::INT { true };

    // if it is a special device-only value (currently `enabled` and `keybinds`) just return true
    if (!orig) Debug::log(WARN, "[device-windowrule] no value '{}' for device '{}' with fallback '{}', assuming `true`", val, dev, fallback);
    return orig ? orig : &true_value;
}

typedef bool (*origDeviceConfigExists)(void*, const std::string& dev);
inline CFunctionHook* g_pDeviceConfigExistsHook = nullptr;
bool hkDeviceConfigExists(void* thisptr, const std::string& dev) {
    return g_pDeviceWindowrules->hasConfig(dev) || (*(origDeviceConfigExists)g_pDeviceConfigExistsHook->m_original)(thisptr, dev);
}

typedef void (*origUpdateLEDs)(IKeyboard*, uint32_t leds);
inline CFunctionHook* g_pUpdateLEDsHook = nullptr;
void hkUpdateLEDs(IKeyboard* thisptr, uint32_t leds) {

    leds |= g_pDeviceWindowrules->getLeds(thisptr->m_hlName);

    (*(origUpdateLEDs)g_pUpdateLEDsHook->m_original)(thisptr, leds);
}

Hyprlang::CParseResult onDeviceFilterKeyword(const char* command, const char* value) {
    Hyprlang::CParseResult res;
    CVarList args(value, 0, ',');

    if (args.size() == 2)
        g_pDeviceWindowrules->registerDeviceFilter(args[0], args[1]);
    else
        res.setError("expected two arguments: name, device-name");

    return res;
}

Hyprlang::CParseResult onDeviceLedKeyword(const char* command, const char* value) {
    Hyprlang::CParseResult res;
    CVarList args(value, 0, ',');
    auto mask = configStringToInt(args[1]);

    if (args.size() == 2 && mask.has_value())
        g_pDeviceWindowrules->registerLedOverride(args[0], mask.value());
    else
        res.setError("expected two arguments: name, led-mask (integer)");

    return res;
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

    // init plugin
    g_pDeviceWindowrules = makeUnique<CDeviceWindowrules>();

    // create config
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_VAR_GLOBAL_LEDS, Hyprlang::INT{0});
    HyprlandAPI::addConfigKeyword(PHANDLE, CONFIG_RULE_FILTER, onDeviceFilterKeyword, Hyprlang::SHandlerOptions {});
    HyprlandAPI::addConfigKeyword(PHANDLE, CONFIG_RULE_LED, onDeviceLedKeyword, Hyprlang::SHandlerOptions {});

    // try hooking
    try {
        g_pGetConfigValueSafeDeviceHook = hook("getConfigValueSafeDevice", "CConfigManager", (void*) &hkGetConfigValueSafeDevice);
        g_pDeviceConfigExistsHook = hook("deviceConfigExists", "CConfigManager", (void*) &hkDeviceConfigExists);

        g_pUpdateLEDsHook = hook("updateLEDsEj", "IKeyboard", (void*) &hkUpdateLEDs); // we wanna hook the one with the args
    } catch (...) {
        HyprlandAPI::addNotification(PHANDLE, "[device-windowrule] Failed to load, hooks could not be made!", CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("hooks failed");
    }

    // register callbacks
    static const auto FOCUS_CALLBACK = HyprlandAPI::registerCallbackDynamic(PHANDLE, "activeWindow", [&](void* self, SCallbackInfo&, std::any data) {
        g_pDeviceWindowrules->updateDevice(std::any_cast<PHLWINDOW>(data));
    });

    static const auto RULE_CHANGE_CALLBACK = HyprlandAPI::registerCallbackDynamic(PHANDLE, "windowUpdateRules", [&](void* self, SCallbackInfo&, std::any data) {
        auto window = std::any_cast<PHLWINDOW>(data);

        // only update device if the window is also focussed
        if (window == g_pCompositor->m_lastWindow)
            g_pDeviceWindowrules->updateDevice(window);
    });

    static const auto PRE_CONFIG_CALLBACK = HyprlandAPI::registerCallbackDynamic( PHANDLE, "preConfigReload", [&](void* self, SCallbackInfo&, std::any data) {
        g_pDeviceWindowrules->clearConfig();
    });

    return {"device-windowrule", "a plugin to apply input device config based on the focused window", "Virt", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() { }

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}
