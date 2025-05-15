# hypr-device-windowrule
This plugin adds a [windowrule](https://wiki.hyprland.org/Configuring/Window-Rules/) which enables you to apply a [device config](https://wiki.hyprland.org/Configuring/Keywords/#per-device-input-configs) to your input devices when a particular window is in focus.

This allows you to change arbitrary device configuration properties based on which window is focused. The main usecase being that you can set keyboard layouts (and options) on a per-window basis. You can however also change other devices, like for example set mouse acceleration curves and sensitivity or disable your touchscreen when a certain window is focussed.

For setting your keyboard layout dynamically, there already exist a couple of solutions which utilize IPC (using the `setxkblayout` dispatcher) to achieve a simliar result. This plugin however can also set other parameters like `kb_options` which these solutions relying on different xkb layouts can't. Also, due to its lower-level nature, this plugin can still make these changes smootly by like for example conserving pressed keys across layout switches. Additionally, it is very convenient to configure using windowrules directly.

As a more interresting feature, this plugin also supports using your keyboard leds to indicate which device config is currently active. So you can for example light up your scrollock led whenever a certain layout is active.

## installation
Installation is supported using `hyprpm`. Hyprland versions starting at `v0.49.0` are supported via commit pins. The main branch targets the latest revision (aka the `-git` package).

```
hyprpm add https://github.com/virtcode/hypr-device-windowrule
hyprpm enable device-windowrule
```

## configuration
For configuration, this plugin piggybacks off of an already existing config feature, [device-specific input configurations](https://wiki.hyprland.org/Configuring/Keywords/#per-device-input-configs). While originally intended to change input settings per-device, with this plugin they are also used for the windowrule settings.

So as a first step, create a new per-device config in your config file. Set the `name` attribute to something that **is NOT already the name of a device you are using**. Like for example:
```ini
device {
    name = my-games-config

    kb_layout = us
    kb_options = ctrl:nocaps
}
```

Note that you can use ***all*** options here that the [device section](https://wiki.hyprland.org/Configuring/Keywords/#per-device-input-configs) supports, not only those which affect the keyboard. For this example though we will stick to keyboard configuration.

To now apply this configuration when a certain window is in focus, you can use the `plugin:device` [windowrule](https://wiki.hyprland.org/Configuring/Window-Rules/) which takes the `name` specified above as an argument. For example:
```ini
windowrule = plugin:device my-games-config, title:^Cyberpunk 2077.*
```

You can have multiple such device sections with different names and different window rules applying them. Note however that **only one device windowrule can be active at a time** (usually the first).

### show status with indicator leds
This plugin supports using the keyboard inicator leds which you already have on your keyboard to show when you are using a device config applied by this plugin.

To enable that, you can use the `deviceleds` keyword which takes the `name` as the first argument, and the **bitmask** applied to the leds as the second. How the bitmask is mapped can be found [in the libinput docs](https://gitlab.freedesktop.org/libinput/libinput/-/blob/main/src/libinput.h#L218). Essentially, add up the following numbers for the leds you want to glow: `1` for numlock, `2` for capslock, `4` for scrollock. As an example:

```ini
plugin:device-windowrule {
    # light up the capslock and scrollock leds when `my-games-config` is active
    deviceleds = my-games-config, 6
}
```

By default, leds will only be changed on devices affected by the changes (see below), and not all connected devices. To change this, see the [`global_leds`](#miscellaneous-options) option.

### only apply to certain devices
As we have used the `name` attribute to identify our device section for the window rule, we can not use it to filter which input device we want to apply the rule to. **By default, the window rule applies to all connected input devices.**

To limit it to certain devices, use the `devicename` keyword. It takes the `name` as the first argument, and then the **actual name of the hardware device** (shown by `hyprctl devices`). This will apply the windowrule only to this hardware device. You can use multiple of these rules for the same `name` to apply it to multiple different devices. Another example:

```ini
plugin:device-windowrule {
    devicename = my-games-config, keyboard-from-vendor-x
    devicename = my-games-config, laptop-keyboard
}

```

### miscellaneous options
The plugin also includes some other config options which can be used to slighty tweak the behaviour of the plugin. They are listed below with their default value.
```ini
plugin:device-windowrule {

    # if set to true, led indicators defined with `deviceleds` will be
    # shown on all devices not just the affected one
    global_leds = false

    # when set to true, the plugin will try its best to conserve the pressed
    # keys and modifiers between layout switches.
    # if you encounter issues which keys getting stuck, try turning this off
    conserve_keys = true

}
```

## license
This plugin is licensed under the 3-Clause BSD license akin to many other hypr* projects. Take a look at the `LICENSE.md` file for more information.
