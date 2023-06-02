import QtQuick
import QtQuick.FigmaStyle
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.RadioButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    spacing: config.spacing || 0

    topPadding: config.topPadding || 0
    bottomPadding: config.bottomPadding || 0
    leftPadding: config.leftPadding || 0
    rightPadding: config.rightPadding || 0

    topInset: -config.topInset || 0
    bottomInset: -config.bottomInset || 0
    leftInset: -config.leftInset || 0
    rightInset: -config.rightInset || 0

    readonly property string currentState: [
        control.checked && "checked",
        !control.enabled && "disabled",
        control.enabled && !control.down && control.hovered && "hovered",
        control.down && "pressed"
    ].filter(Boolean).join("-") || "normal"
    readonly property var config: ConfigReader.controls.radiobutton[currentState] || {}
    readonly property bool mirroredIndicator: control.mirrored !== (config.mirrored || false)

    indicator: Image {
        x: control.text ? (control.mirroredIndicator ? control.width - width - control.rightPadding : control.leftPadding) : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        source: control.config.indicator_background?.export === "image"
            ? Qt.resolvedUrl("images/" + control.config.indicator_background.fileName)
            : ""

        Image {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            source: control.config.indicator?.export === "image"
                    ? Qt.resolvedUrl("images/" + control.config.indicator.fileName)
                    : ""
        }
    }

    contentItem: Text {
        leftPadding: control.indicator && !control.mirrored ? control.indicator.width + control.spacing : 0
        rightPadding: control.indicator && control.mirrored ? control.indicator.width + control.spacing : 0

        text: control.text
        font: control.font
        color: control.palette.windowText
        elide: Text.ElideRight
        horizontalAlignment: control.config.label.textHAlignment
        verticalAlignment: control.config.label.textVAlignment
    }

    background: BorderImage {
        source: control.config.background?.export === "image"
                    ? Qt.resolvedUrl("images/" + control.config.background.fileName)
                    : ""
        border {
            top: control.config.background?.topOffset || 0
            bottom: control.config.background?.bottomOffset || 0
            left: control.config.background?.leftOffset || 0
            right: control.config.background?.rightOffset || 0
        }
    }
}
