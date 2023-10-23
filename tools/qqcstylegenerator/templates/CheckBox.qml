import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.CheckBox {
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

    readonly property string __currentState: [
        control.checkState === Qt.Checked && "checked",
        !control.enabled && "disabled",
        control.enabled && !control.down && control.hovered && "hovered",
        control.checkState === Qt.PartiallyChecked && "partiallyChecked",
        control.down && "pressed",
    ].filter(Boolean).join("_") || "normal"
    readonly property var config: Config.controls.checkbox[__currentState] || {}
    readonly property bool mirroredIndicator: control.mirrored !== (config.mirrored || false)

    indicator: Image {
        x: control.text ? (control.mirroredIndicator ? control.width - width - control.rightPadding : control.leftPadding) : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        source: Qt.resolvedUrl(control.config.indicator.filePath)
    }

    contentItem: Text {
        leftPadding: control.indicator && !control.mirroredIndicator ? control.indicator.width + control.spacing : 0
        rightPadding: control.indicator && control.mirroredIndicator ? control.indicator.width + control.spacing : 0

        text: control.text
        font: control.font
        color: control.palette.windowText
        elide: Text.ElideRight
        horizontalAlignment: control.config.label.textHAlignment
        verticalAlignment: control.config.label.textVAlignment
    }

    background: StyleImage {
        imageConfig: control.config.background
    }
}
