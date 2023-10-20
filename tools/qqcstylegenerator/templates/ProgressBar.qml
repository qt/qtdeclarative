import QtQuick
import QtQuick.FigmaStyle
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.ProgressBar {
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

    readonly property string __currentState: [
        !control.enabled && "disabled",
        control.indeterminate && "indeterminate"
    ].filter(Boolean).join("_") || "normal"
    readonly property var config: Config.controls.progressbar[__currentState] || {}

    contentItem: Item {
        parent: control.background
        implicitWidth: control.indeterminate ? animatedProgress.implicitWidth : progress.implicitWidth
        implicitHeight: control.indeterminate ? animatedProgress.implicitHeight : progress.implicitHeight
        scale: control.mirrored ? -1 : 1

        readonly property StyleImage progress: StyleImage {
            parent: control.contentItem
            visible: !control.indeterminate && control.value
            width: control.position * parent.width
            height: parent.height
            imageConfig: control.config.track
        }

        readonly property StyleImage animatedProgress: StyleImage {
            parent: control.contentItem
            visible: control.indeterminate
            width: implicitWidth
            height: parent.height
            imageConfig: control.config.track

            SequentialAnimation on x {
                loops: Animation.Infinite
                SmoothedAnimation { from: 0; to: control.width - control.contentItem.animatedProgress.width; velocity: 200}
                SmoothedAnimation { from: control.width - control.contentItem.animatedProgress.width; to: 0; velocity: 200}
            }
        }
    }

    background: StyleImage {
        imageConfig: control.config.groove
    }
}
