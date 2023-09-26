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

    topPadding: config.topPadding || 0
    leftPadding: config.leftPadding || 0
    rightPadding: config.rightPadding || 0
    bottomPadding: config.bottomPadding || 0

    topInset: -config.topInset || 0
    bottomInset: -config.bottomInset || 0
    leftInset: -config.leftInset || 0
    rightInset: -config.rightInset || 0

    readonly property var config: ConfigReader.controls.scrollindicator["normal"] || {}

    contentItem: Item {
        implicitWidth: control.horizontal ? _handle.implicitWidth : _handle.implicitHeight
        implicitHeight: control.horizontal ? _handle.implicitHeight : _handle.implicitWidth
        opacity: 0.0

        property BorderImage _handle: BorderImage {
            parent: control.contentItem
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            width: control.horizontal ? control.availableWidth : control.availableHeight
            height: control.horizontal ? control.availableHeight : control.availableWidth
            rotation: control.horizontal ? 0 : -90
            source: Qt.resolvedUrl(control.config.handle.filePath)
            border {
                top: control.config.handle?.topOffset || 0
                bottom: control.config.handle?.bottomOffset || 0
                left: control.config.handle?.leftOffset || 0
                right: control.config.handle?.rightOffset || 0
            }
        }
    }

    background: Item {
        implicitWidth: control.horizontal ? _background.implicitWidth : _background.implicitHeight
        implicitHeight: control.horizontal ? _background.implicitHeight : _background.implicitWidth
        opacity: 0.0

        property BorderImage _background: BorderImage {
            parent: control.background
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            width: control.horizontal ? control.background.width : background.height
            height: control.horizontal ? control.background.height : background.width
            rotation: control.horizontal ? 0 : -90
            source: Qt.resolvedUrl(control.config.background.filePath)
            border {
                top: control.config.background?.topOffset || 0
                bottom: control.config.background?.bottomOffset || 0
                left: control.config.background?.leftOffset || 0
                right: control.config.background?.rightOffset || 0
            }
        }
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
