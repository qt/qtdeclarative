import Qt.labs.controls.universal 1.0

import "../.."

Controls {
    color: Universal.backgroundColor
    Universal.theme: themeSwitch.checked ? Universal.Dark : Universal.Light
}

