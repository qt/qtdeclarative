import QtQuick
import QtQuick.FigmaStyle
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.Switch {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitIndicatorWidth)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    spacing: config.spacing || 0

    topPadding: config.topPadding || 0
    leftPadding: background.width + spacing
    rightPadding: spacing
    bottomPadding: config.bottomPadding || 0

    topInset: -config.topInset || 0
    leftInset: -config.leftInset || 0
    rightInset: (-config.rightInset || 0) + (2 * spacing + implicitContentWidth + rightPadding)
    bottomInset: -config.bottomInset || 0

    readonly property string currentState: [
        control.checked && "checked",
        !control.enabled && "disabled",
        control.visualFocus && "focused",
        control.enabled && !control.down && control.hovered && "hovered",
        control.down && "pressed",
    ].filter(Boolean).join("-") || "normal"
    readonly property var config: ConfigReader.controls.switch[currentState] || {}

    indicator: Item {
        implicitWidth: Math.max(handle.width, background.implicitWidth)
        implicitHeight: Math.max(handle.height, background.implicitHeight)

        property BorderImage handle: BorderImage {
            parent: control.indicator
            x: Math.max(0, Math.min(parent.width - width, control.visualPosition * parent.width - (width / 2)))
            y: (parent.height - height) / 2
            source: control.config.handle?.export === "image"
                        ? Qt.resolvedUrl("images/" + control.config.handle.name)
                        : ""
            border {
                top: control.config.handle?.topOffset || 0
                bottom: control.config.handle?.bottomOffset || 0
                left: control.config.handle?.leftOffset || 0
                right: control.config.handle?.rightOffset || 0
            }

            Behavior on x {
                enabled: !control.down
                SmoothedAnimation {
                    velocity: 200
                }
            }

            Image {
                x: (parent.width - width) / 2
                y: (parent.height - height) / 2
                source: control.config[handle]?.export === "image"
                            ? Qt.resolvedUrl("images/" + control.config[handle].name)
                            : ""
                readonly property string handle: "handle_" + (control.checked ? "righticon" : "lefticon")
            }
        }
    }

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.palette.windowText
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    background: BorderImage {
        source: control.config.background?.export === "image"
                    ? Qt.resolvedUrl("images/" + control.config.background.name)
                    : ""
        border {
            top: control.config.background?.topOffset || 0
            bottom: control.config.background?.bottomOffset || 0
            left: control.config.background?.leftOffset || 0
            right: control.config.background?.rightOffset || 0
        }

        Image {
            // TODO: The exact position on the icon should be set from the json config file
            x: (control.indicator.handle.width - width) / 2
            y: (parent.height - height) / 2
            source: control.config.background_lefticon?.export === "image"
                        ? Qt.resolvedUrl("images/" + control.config.background_lefticon.name)
                        : ""
        }

        Image {
            // TODO: The exact position on the icon should be set from the json config file
            x: parent.width - control.indicator.handle.width + (control.indicator.handle.width / 2)
            y: (parent.height - height) / 2
            source: control.config.background_righticon?.export === "image"
                        ? Qt.resolvedUrl("images/" + control.config.background_righticon.name)
                        : ""
        }
    }
}
