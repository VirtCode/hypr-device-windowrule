#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>

#define CONFIG_WINDOWRULE           "device"

#define CONFIG_RULE_FILTER          "devicename"
#define CONFIG_RULE_LED             "deviceleds"

#define CONFIG_VAR_GLOBAL_LEDS      "plugin:device-windowrule:global_leds"
#define CONFIG_VAR_CONSERVE_KEYS    "plugin:device-windowrule:conserve_keys"

inline HANDLE PHANDLE = nullptr;
