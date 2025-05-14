#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <cstdint>
#include <hyprlang.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class CDeviceWindowrules {
  private:
    /* currently selected device rule */
    std::optional<std::string> m_selected;

    /* devices allowed to use a rule */
    std::unordered_map<std::string, std::vector<std::string>> m_devices;
    /* led mask applied when a rule is used */
    std::unordered_map<std::string, uint32_t> m_leds;

  public:
    /* should be called when a new window is focussed or the focussed window changes */
    void updateDevice(const PHLWINDOW);

    /* get the config value for the current device or `nullptr` if nothing changed */
    Hyprlang::CConfigValue* getConfig(const std::string& dev, const std::string& val) const;
};

inline UP<CDeviceWindowrules> g_pDeviceWindowrules;
