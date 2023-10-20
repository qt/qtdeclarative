import QtQuick
import QtQuick.FigmaStyle
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.TabButton {
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

    readonly property string __currentState: [
        checked && "checked",
        !enabled && "disabled",
        enabled && !down && hovered && "hovered",
        down && "pressed"
    ].filter(Boolean).join("_") || "normal"
    readonly property var config: Config.controls.tabbutton[__currentState] || {}

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        alignment: control.config.label.textVAlignment | control.config.label.textHAlignment
        icon: control.icon
        text: control.text
        font: control.font
        color: control.palette.buttonText
    }

    background: StyleImage {
        imageConfig: control.config.background
    }
}
