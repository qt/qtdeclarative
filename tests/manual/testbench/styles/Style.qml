import Qt.labs.controls 1.0

import ".."

Controls {
    color: Theme.backgroundColor
    Theme.backgroundColor: themeSwitch.checked ? "#444" : "#fff"
    Theme.frameColor: themeSwitch.checked ? "#666" : "#ccc"
    Theme.textColor: themeSwitch.checked ? "#eee" : "#111"
    Theme.pressColor: themeSwitch.checked ? "#33ffffff" : "#33333333"
    Theme.baseColor: themeSwitch.checked ? "#444" : "#eee"
}
