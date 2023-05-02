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

    readonly property var config: ConfigReader.configForImageUrl(background?.source ?? "")

    spacing: config?.spacing || 0

    topPadding: config?.topPadding || 0
    leftPadding: background.width + spacing
    rightPadding: spacing
    bottomPadding: config?.bottomPadding || 0

    topInset: -config?.topInset || 0
    leftInset: -config?.leftInset || 0
    rightInset: (-config?.rightInset || 0) + (2 * spacing + implicitContentWidth + rightPadding)
    bottomInset: -config?.bottomInset || 0

    indicator: Item {
        implicitWidth: Math.max(handle.width, background.implicitWidth)
        implicitHeight: Math.max(handle.height, background.implicitHeight)

        property BorderImage handle: BorderImage {
            parent: control.indicator
            x: Math.max(0, Math.min(parent.width - width, control.visualPosition * parent.width - (width / 2)))
            y: (parent.height - height) / 2
            source: Qt.resolvedUrl("images/switch-handle")
            border.top: config.topOffset || 0
            border.bottom: config.bottomOffset || 0
            border.left: config.leftOffset || 0
            border.right: config.rightOffset || 0

            readonly property var config: ConfigReader.configForImageUrl(source)

            Behavior on x {
                enabled: !control.down
                SmoothedAnimation {
                    velocity: 200
                }
            }

            ImageSelector on source {
                states: [
                    {"disabled": !control.enabled},
                    {"pressed": control.down},
                    {"checked": control.checked},
                    {"focused": control.visualFocus},
                    {"mirrored": control.mirrored},
                    {"hovered": control.enabled && control.hovered}
                ]
            }

            Image {
                x: (parent.width - width) / 2
                y: (parent.height - height) / 2
                source: Qt.resolvedUrl("images/switch-handle")
                ImageSelector on source {
                    states: [
                        {"righticon": control.checked},
                        {"lefticon": !control.checked},
                        {"disabled": !control.enabled},
                        {"pressed": control.down},
                        {"checked": control.checked},
                        {"focused": control.visualFocus},
                        {"mirrored": control.mirrored},
                        {"hovered": control.enabled && control.hovered}
                    ]
                }
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
        source: Qt.resolvedUrl("images/switch-background")

        border.top: control.config?.topOffset || 0
        border.bottom: control.config?.bottomOffset || 0
        border.left: control.config?.leftOffset || 0
        border.right: control.config?.rightOffset || 0

        ImageSelector on source {
            states: [
                {"disabled": !control.enabled},
                {"pressed": control.down},
                {"checked": control.checked},
                {"focused": control.visualFocus},
                {"mirrored": control.mirrored},
                {"hovered": control.enabled && control.hovered}
            ]
        }

        Image {
            // TODO: The exact position on the icon should be set from the json config file
            x: (control.indicator.handle.width - width) / 2
            y: (parent.height - height) / 2
            source: Qt.resolvedUrl("images/switch-background-lefticon")
            ImageSelector on source {
                states: [
                    {"disabled": !control.enabled},
                    {"pressed": control.down},
                    {"checked": control.checked},
                    {"focused": control.visualFocus},
                    {"mirrored": control.mirrored},
                    {"hovered": control.enabled && control.hovered}
                ]
            }
        }

        Image {
            // TODO: The exact position on the icon should be set from the json config file
            x: parent.width - control.indicator.handle.width + (control.indicator.handle.width / 2)
            y: (parent.height - height) / 2
            source: Qt.resolvedUrl("images/switch-background-righticon")
            ImageSelector on source {
                states: [
                    {"disabled": !control.enabled},
                    {"pressed": control.down},
                    {"checked": control.checked},
                    {"focused": control.visualFocus},
                    {"mirrored": control.mirrored},
                    {"hovered": control.enabled && control.hovered}
                ]
            }
        }
    }
}
