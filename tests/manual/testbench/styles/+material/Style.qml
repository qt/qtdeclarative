import Qt.labs.controls.material 1.0

import "../.."

Controls {
    color: Material.backgroundColor
    Material.theme: themeSwitch.checked ? Material.Dark : Material.Light
}
