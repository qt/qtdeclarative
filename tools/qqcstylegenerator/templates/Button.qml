import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.Button {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    spacing: config.spacing || 0

    topPadding: config.topPadding || 0
    bottomPadding: config.bottomPadding || 0
    leftPadding: config.leftPadding || 0
    rightPadding: config.rightPadding || 0

    topInset: -config.topInset || 0
    bottomInset: -config.bottomInset || 0
    leftInset: -config.leftInset || 0
    rightInset: -config.rightInset || 0

    icon.width: 17
    icon.height: 17
    icon.color: control.flat ? (control.down ? control.palette.highlight : control.palette.windowText)
                             : control.palette.buttonText

    readonly property string __currentState: [
        control.checked && "checked",
        !control.enabled && "disabled",
        control.enabled && !control.down && control.hovered && "hovered",
        control.down && "pressed"
    ].filter(Boolean).join("_") || "normal"
    readonly property var config: (control.flat && Config.controls.flatbutton
        ? Config.controls.flatbutton[__currentState]
        : Config.controls.button[__currentState]) || {}

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        alignment: control.config.label.textVAlignment | control.config.label.textHAlignment
        icon: control.icon
        text: control.text
        font: control.font
        color: control.icon.color
    }

    background: StyleImage {
        imageConfig: control.config.background
    }
}
