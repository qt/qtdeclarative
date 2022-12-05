import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import "sub" as Sub

Item {
    Image { source: "a.png" }
    IconLabel { icon.source: "a.png" }
    Button {
        icon.color: "transparent"
        icon.source: "a.png"
    }
    Button {
        action: actions.action
        icon.color: "transparent"
        Sub.Actions { id: actions }
    }
}
