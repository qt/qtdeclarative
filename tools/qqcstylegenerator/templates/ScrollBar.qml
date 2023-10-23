import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.ScrollBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    visible: control.policy !== T.ScrollBar.AlwaysOff
    minimumSize: orientation === Qt.Horizontal ? height / width : width / height

    topPadding: (horizontal ? config.topPadding : config.leftPadding) || 0
    leftPadding: (horizontal ? config.leftPadding : config.bottomPadding) || 0
    rightPadding: (horizontal ? config.rightPadding : config.topPadding) || 0
    bottomPadding: (horizontal ? config.bottomPadding : config.rightPadding) || 0

    topInset: (horizontal ? -config.topInset : -config.leftInset) || 0
    leftInset: (horizontal ? -config.leftInset : -config.bottomInset) || 0
    rightInset: (horizontal ? -config.rightInset : -config.topInset) || 0
    bottomInset: (horizontal ? -config.bottomInset : -config.rightInset) || 0

    readonly property string __currentState: [
        !control.enabled && "disabled",
        control.enabled && !control.pressed && control.hovered && "hovered",
        control.pressed && "pressed"
    ].filter(Boolean).join("_") || "normal"
    readonly property var config: Config.controls.scrollbar[__currentState] || {}

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
            when: control.policy === T.ScrollBar.AlwaysOn || (control.active && control.size < 1.0)
        }
    ]

    transitions: [
        // TODO: Set transition settings based on Figma prototype
        Transition {
            to: "active"
            NumberAnimation { targets: [control.contentItem, control.background]; property: "opacity"; to: 1.0 }
        },
        Transition {
            from: "active"
            SequentialAnimation {
                PropertyAction{ targets: [control.contentItem, control.background]; property: "opacity"; value: 1.0 }
                PauseAnimation { duration: 3000 }
                NumberAnimation { targets: [control.contentItem, control.background]; property: "opacity"; to: 0.0 }
            }
        }
    ]
}
