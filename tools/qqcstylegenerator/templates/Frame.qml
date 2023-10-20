import QtQuick
import QtQuick.FigmaStyle
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.Frame {
    id: control

    implicitWidth: Math.max((background.minimumWidth || implicitBackgroundWidth)
                            + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max((background.minimumHeight || implicitBackgroundHeight)
                            + topInset + bottomInset,
                            contentHeight + topPadding + bottomPadding)

    topPadding: config.topPadding || 0
    bottomPadding: config.bottomPadding || 0
    leftPadding: config.leftPadding || 0
    rightPadding: config.rightPadding || 0

    topInset: -config.topInset || 0
    bottomInset: -config.bottomInset || 0
    leftInset: -config.leftInset || 0
    rightInset: -config.rightInset || 0

    readonly property string __currentState: !control.enabled ? "disabled" : "normal";
    readonly property var config: Config.controls.frame[__currentState] || {}

    background: StyleImage {
        imageConfig: control.config.background
    }
}
