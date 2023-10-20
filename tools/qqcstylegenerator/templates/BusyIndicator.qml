import QtQuick
import QtQuick.FigmaStyle
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.BusyIndicator {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    topPadding: config.topPadding || 0
    bottomPadding: config.bottomPadding || 0
    leftPadding: config.leftPadding || 0
    rightPadding: config.rightPadding || 0

    topInset: -config.topInset || 0
    bottomInset: -config.bottomInset || 0
    leftInset: -config.leftInset || 0
    rightInset: -config.rightInset || 0

    readonly property string __currentState: !control.enabled ? "disabled" : "normal"
    readonly property var config: Config.controls.busyindicator[__currentState] || {}

    contentItem: StyleImage {
        imageConfig: control.config.indicator
        transformOrigin: Item.Center
        NumberAnimation on rotation {
            running: control.visible && control.running
            from: 0; to: 360;
            loops: Animation.Infinite;
            duration: 1200
        }
    }

    background: StyleImage {
        imageConfig: control.config.background
    }
}

