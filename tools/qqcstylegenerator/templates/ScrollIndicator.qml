import QtQuick
import QtQuick.FigmaStyle
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.ScrollIndicator {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    topPadding: (horizontal ? config.topPadding : config.leftPadding) || 0
    leftPadding: (horizontal ? config.leftPadding : config.bottomPadding) || 0
    rightPadding: (horizontal ? config.rightPadding : config.topPadding) || 0
    bottomPadding: (horizontal ? config.bottomPadding : config.rightPadding) || 0

    topInset: (horizontal ? -config.topInset : -config.leftInset) || 0
    leftInset: (horizontal ? -config.leftInset : -config.bottomInset) || 0
    rightInset: (horizontal ? -config.rightInset : -config.topInset) || 0
    bottomInset: (horizontal ? -config.bottomInset : -config.rightInset) || 0

    readonly property var config: Config.controls.scrollindicator["normal"] || {}

    contentItem: StyleImage {
        imageConfig: control.config.handle
        horizontal: control.horizontal
        width: control.availableWidth
        height: control.availableHeight
        opacity: 0.0
    }

    background: StyleImage {
        imageConfig: control.config.background
        horizontal: control.horizontal
        opacity: 0.0
    }

    states: [
        State {
            name: "active"
            when: control.active && control.size < 1.0
        }
    ]

    transitions: [
        // TODO: Set transition based on Figma prototype
        Transition {
            to: "active"
            NumberAnimation { targets: [control.contentItem, control.background]; property: "opacity"; to: 1.0 }
        },
        Transition {
            from: "active"
            SequentialAnimation {
                PauseAnimation { duration: 5000 }
                NumberAnimation { targets: [control.contentItem, control.background]; property: "opacity"; to: 0.0 }
            }
        }
    ]
}
