import QtQuick
import QtQuick.FigmaStyle
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.Frame {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    topPadding: config.topPadding || 0
    bottomPadding: config.bottomPadding || 0
    leftPadding: config.leftPadding || 0
    rightPadding: config.rightPadding || 0

    topInset: -config.topInset || 0
    bottomInset: -config.bottomInset || 0
    leftInset: -config.leftInset || 0
    rightInset: -config.rightInset || 0

    readonly property string currentState: !control.enabled ? "disabled" : "normal";
    readonly property var config: ConfigReader.controls.frame[currentState] || {}

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
