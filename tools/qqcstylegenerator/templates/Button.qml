import QtQuick
import QtQuick.FigmaStyle
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
    icon.color: control.flat ? (control.enabled ? (control.down ? control.palette.highlight : control.palette.button)
                                                : control.palette.mid)
                             : control.palette.buttonText

    readonly property string currentState: [
        control.checked && "checked",
        !control.enabled && "disabled",
        control.enabled && !control.down && control.hovered && "hovered",
        control.down && "pressed"
    ].filter(Boolean).join("-") || "normal"
    readonly property var config: ConfigReader.controls.button[currentState] || {}

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

    background: BorderImage {
        source: control.config.background?.export === "image"
                ? Qt.resolvedUrl(control.config.background.filePath)
                : ""

        border {
            top: control.config.background?.topOffset || 0
            bottom: control.config.background?.bottomOffset || 0
            left: control.config.background?.leftOffset || 0
            right: control.config.background?.rightOffset || 0
        }
    }
}
