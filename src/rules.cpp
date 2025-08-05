#include "rules.hpp"
#include "globals.hpp"

#define private public
#define protected public
#include <hyprland/src/devices/IKeyboard.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#undef private
#undef protected

#include <hyprland/src/managers/input/InputManager.hpp>

void CDeviceWindowrules::updateDevice(const PHLWINDOW window) {
    static auto* const* PREPLAY = (Hyprlang::INT* const*) HyprlandAPI::getConfigValue(PHANDLE, CONFIG_VAR_CONSERVE_KEYS)->getDataStaticPtr();

    auto last = m_selected;
    bool set = false;

    if (window) for (auto rule : window->m_matchedRules) {
        if (rule->m_ruleType == CWindowRule::RULE_PLUGIN && rule->m_rule.starts_with(CONFIG_WINDOWRULE)) {
            auto device = CVarList(rule->m_rule, 0, ' ')[1];

            Debug::log(LOG, "[device-windowrule] setting device to {}", device);

            if (device.empty()) m_selected = {};
            else m_selected = device;

            set = true;
            break;
        }
    }

    // unset if no window rule
    if (!set) m_selected = {};

    if (last != m_selected) {
        Debug::log(LOG, "[device-windowrule] changing input device config from {} to {}", last.value_or("<none>"), m_selected.value_or("<none>"));

        // see HyprCtl.cpp line 1119
        g_pInputManager->setKeyboardLayout();     // update kb layout
        g_pInputManager->setPointerConfigs();     // update mouse cfgs
        g_pInputManager->setTouchDeviceConfigs(); // update touch device cfgs
        g_pInputManager->setTabletConfigs();      // update tablets

        for (auto const& keyboard : g_pInputManager->m_keyboards) {
            if (**PREPLAY) { // we replay for _all_ keyboards cause the layout will be reloaded for every one (even if no changes)
                auto original = keyboard->m_pressed;
                keyboard->m_pressed.clear();

                // we replay all pressed keys to make sure modifiers etc. are updated correctly again
                // just setting the modifiers again is not sufficient for some reason
                for (uint32_t key : original) {
                    keyboard->updatePressed(key, true);
                    keyboard->updateXkbStateWithKey(key + 8, true); // this +8 converts the keycode to an xkb keycode, as we now store the normal ones
                }
            }

            // also update leds
            keyboard->updateLEDs();
        }
    }
}

Hyprlang::CConfigValue* CDeviceWindowrules::getConfig(const std::string& dev, const std::string& val) const {
    if (m_selected.has_value()) {

        /// only use custom rule if whitelist is empty or device is whitelisted
        if (m_devices.contains(m_selected.value()) && !m_devices.at(m_selected.value()).contains(dev))
            return nullptr;

        const auto VAL = g_pConfigManager->m_config->getSpecialConfigValuePtr("device", val.c_str(), m_selected->c_str());

        if (VAL && VAL->m_bSetByUser)
            return VAL;
    }

    return nullptr;
}

bool CDeviceWindowrules::hasConfig(const std::string& dev) const {
    return m_selected.has_value() && (!m_devices.contains(m_selected.value()) || m_devices.at(m_selected.value()).contains(dev));
}

uint32_t CDeviceWindowrules::getLeds(const std::string& dev) const {
    static auto* const* PGLOBAL = (Hyprlang::INT* const*) HyprlandAPI::getConfigValue(PHANDLE, CONFIG_VAR_GLOBAL_LEDS)->getDataStaticPtr();

    if (m_selected.has_value() && m_leds.contains(m_selected.value())) {

        // check that the device is also affected by the changes
        if (**PGLOBAL || !m_devices.contains(m_selected.value()) || m_devices.at(m_selected.value()).contains(dev))
            return m_leds.at(m_selected.value());
    }

    return 0; // no changes, they are |-ed
}

void CDeviceWindowrules::registerLedOverride(const std::string& rule, const uint32_t leds) {
    m_leds.insert({rule, leds});
}

void CDeviceWindowrules::registerDeviceFilter(const std::string& rule, const std::string& dev) {
    if (m_devices.contains(rule)) m_devices.at(rule).insert(dev);
    else m_devices.insert({rule, {dev}});
}

void CDeviceWindowrules::clearConfig() {
    m_devices.clear();
    m_leds.clear();
}
