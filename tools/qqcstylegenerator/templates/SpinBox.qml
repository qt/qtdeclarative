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

    leftPadding: (config ? config.spacing + config.leftPadding : 0)
        + (mirroredIndicators
        ? (up.indicator ? up.indicator.width : 0)
        : (down.indicator ? down.indicator.width : 0))
    rightPadding: (config ? config.spacing + config.rightPadding : 0)
        + (mirroredIndicators
        ? (down.indicator ? down.indicator.width : 0)
        : (up.indicator ? up.indicator.width : 0))
    topPadding: config?.topPadding || 0
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

    up.indicator: Image {
        x: control.mirroredIndicators
            ? (control.upConfig?.leftPadding || 0)
            : control.width - width - (control.upConfig?.rightPadding || 0)
        y: (control.upConfig?.topPadding || 0)
            + (control.height - (control.upConfig ? control.upConfig.topPadding + control.upConfig.bottomPadding : 0) - height) / 2
        source: Qt.resolvedUrl(control.upConfig.indicator_up.filePath)
    }

    down.indicator: Image {
        x: control.mirroredIndicators
            ? control.width - width - (control.downConfig?.rightPadding || 0)
            : (control.downConfig?.leftPadding || 0)
        y: (control.downConfig?.topPadding || 0)
            + (control.height - (control.downConfig ? control.downConfig.topPadding + control.downConfig.bottomPadding : 0) - height) / 2
        source: Qt.resolvedUrl(control.downConfig.indicator_down.filePath)
    }

    background: StyleImage {
        imageConfig: control.config.background
    }
}
