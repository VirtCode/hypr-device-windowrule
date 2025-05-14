#include <hyprgraphics/color/Color.hpp>
#include <hyprland/src/helpers/memory/Memory.hpp>
#include <hyprland/src/plugins/HookSystem.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprlang.hpp>
#include <optional>
#include <string>
#include <unistd.h>

#include "globals.hpp"

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
    if (HASH != GIT_COMMIT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, "[device-windowrule] Failed to load, mismatched headers!", CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        HyprlandAPI::addNotification(PHANDLE, std::format("[device-windowrule] Built with: {}, running: {}", GIT_COMMIT_HASH, HASH), CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("version mismatch");
    }

    // try hooking
    try {
    } catch (...) {
        HyprlandAPI::addNotification(PHANDLE, "[device-windowrule] Failed to load, hooks could not be made!", CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("hooks failed");
    }

    return {"device-windowrule", "a plugin to apply input device config based on the focused window", "Virt", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() { }

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}
