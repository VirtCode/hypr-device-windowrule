#include "rules.hpp"
#include <hyprland/src/managers/input/InputManager.hpp>

#define private public
#include <hyprland/src/config/ConfigManager.hpp>
#undef private

void CDeviceWindowrules::updateDevice(const PHLWINDOW window) {
    auto last = m_selected;
    bool set = false;

    for (auto rule : window->m_matchedRules) {
        if (rule->m_ruleType == CWindowRule::RULE_PLUGIN && rule->m_rule.starts_with("plugin:device")) {
            auto device = CVarList(rule->m_rule, 0, ' ')[1];

            Debug::log(ERR, "[device-windowrule] setting device to {}", device);

            if (device.empty()) m_selected = {};
            else m_selected = device;

            set = true;
            break;
        }
    }

    // unset if no window rule
    if (!set) m_selected = {};

    if (last != m_selected) {
        Debug::log(ERR, "[device-windowrule] re-setting input device settings");

        // see HyprCtl.cpp line 1119
        g_pInputManager->setKeyboardLayout();     // update kb layout
        g_pInputManager->setPointerConfigs();     // update mouse cfgs
        g_pInputManager->setTouchDeviceConfigs(); // update touch device cfgs
        g_pInputManager->setTabletConfigs();      // update tablets
    }
}

Hyprlang::CConfigValue* CDeviceWindowrules::getConfig(const std::string& dev, const std::string& val) const {
    if (m_selected.has_value()) {
        const auto VAL = g_pConfigManager->m_config->getSpecialConfigValuePtr("device", val.c_str(), m_selected->c_str());

        if (VAL && VAL->m_bSetByUser)
            return VAL;
    }

    return nullptr;
}
