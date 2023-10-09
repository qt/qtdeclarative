import QtQuick
import QtQuick.FigmaStyle
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.SpinBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             up.implicitIndicatorHeight, down.implicitIndicatorHeight)

    property string __controlState: [
        enabled && hovered && (down.hovered || down.pressed) && "down",
        enabled && hovered && (up.hovered || up.pressed) && "up",
        enabled && (down.pressed || up.pressed) && "pressed",
        enabled && hovered && !(down.pressed || up.pressed) && "hovered"
    ].filter(Boolean).join("-") || "normal"
    readonly property var config: ConfigReader.controls.spinbox[__controlState] || {}
    readonly property var downConfig: value == from ? ConfigReader.controls.spinbox["atlimit"] : config
    readonly property var upConfig: value == to ? ConfigReader.controls.spinbox["atlimit"] : config

    readonly property bool mirroredIndicators: control.mirrored !== (config.mirrored || false)

    leftPadding: config.leftPadding || 0
    rightPadding: config.rightPadding || 0
    topPadding: config.topPadding || 0
    bottomPadding: config?.bottomPadding || 0

    topInset: -config?.topInset || 0
    bottomInset: -config?.bottomInset || 0
    leftInset: -config?.leftInset || 0
    rightInset: -config?.rightInset || 0

    validator: IntValidator {
        locale: control.locale.name
        bottom: Math.min(control.from, control.to)
        top: Math.max(control.from, control.to)
    }

    contentItem: TextInput {
        clip: width < implicitWidth
        text: control.displayText
        opacity: control.enabled ? 1 : 0.3

        font: control.font
        color: control.palette.text
        selectionColor: control.palette.highlight
        selectedTextColor: control.palette.highlightedText
        horizontalAlignment: control.config.textInput.textHAlignment
        verticalAlignment: control.config.textInput.textVAlignment

        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: control.inputMethodHints
    }

    down.indicator: StyleImage {
        height: control.height
        imageConfig: control.downConfig.indicator_down_background

        StyleImage {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            imageConfig: control.mirrored
                ? control.upConfig.indicator_up_icon
                : control.downConfig.indicator_down_icon
        }
    }

    up.indicator: StyleImage {
        x: control.width - width
        height: control.height
        imageConfig: control.upConfig.indicator_up_background

        StyleImage {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            imageConfig: control.mirrored
                ? control.downConfig.indicator_down_icon
                : control.upConfig.indicator_up_icon
        }
    }

    background: StyleImage {
        imageConfig: control.config.background
    }
}
