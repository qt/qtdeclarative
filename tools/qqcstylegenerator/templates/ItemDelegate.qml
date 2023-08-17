import QtQuick
import QtQuick.FigmaStyle
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.ItemDelegate {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    spacing: config.spacing || 0

    topPadding: config.topPadding || 0
    leftPadding: config.leftPadding || 0
    rightPadding: config.rightPadding || 0
    bottomPadding: config.bottomPadding || 0

    topInset: -config.topInset || 0
    bottomInset: -config.bottomInset || 0
    leftInset: -config.leftInset || 0
    rightInset: -config.rightInset || 0

    icon.width: 17
    icon.height: 17
    icon.color: control.palette.text

    readonly property string __currentState: [
        !control.enabled && "disabled",
        control.highlighted && "highlighted",
        control.enabled && !control.down && control.hovered && "hovered",
        control.down && "pressed"
    ].filter(Boolean).join("-") || "normal"
    readonly property var config: ConfigReader.controls.itemdelegate[__currentState] || {}

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        alignment: control.config.label.textVAlignment | config.label.textHAlignment

        icon: control.icon
        text: control.text
        font: control.font
        color: control.palette.text
    }

    background: StyleImage {
        imageConfig: control.config.background
    }
}
