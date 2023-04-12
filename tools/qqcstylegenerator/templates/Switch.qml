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

    readonly property string backgroundName: "switch-background"

    spacing: ConfigReader.images[backgroundName].spacing

    topPadding: ConfigReader.images[backgroundName].topPadding
    leftPadding: background.width + spacing
    rightPadding: spacing
    bottomPadding: ConfigReader.images[backgroundName].bottomPadding

    topInset: -ConfigReader.images[backgroundName].topInset || 0
    leftInset: -ConfigReader.images[backgroundName].leftInset || 0
    rightInset: (-ConfigReader.images[backgroundName].rightInset || 0) + (2 * spacing + implicitContentWidth + rightPadding)
    bottomInset: -ConfigReader.images[backgroundName].bottomInset || 0

    indicator: Item {
        implicitWidth: Math.max(handle.width, background.implicitWidth)
        implicitHeight: Math.max(handle.height, background.implicitHeight)

        property BorderImage handle: BorderImage {
            parent: control.indicator
            x: Math.max(0, Math.min(parent.width - width, control.visualPosition * parent.width - (width / 2)))
            y: (parent.height - height) / 2

            Behavior on x {
                enabled: !control.down
                SmoothedAnimation {
                    velocity: 200
                }
            }

            readonly property string name: "switch-handle"
            source: Qt.resolvedUrl("images/" + name)

            border.top: ConfigReader.images[name].topOffset || 0
            border.bottom: ConfigReader.images[name].bottomOffset || 0
            border.left: ConfigReader.images[name].leftOffset || 0
            border.right: ConfigReader.images[name].rightOffset || 0

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
        source: Qt.resolvedUrl("images/" + control.backgroundName)

        border.top: ConfigReader.images[control.backgroundName].topOffset || 0
        border.bottom: ConfigReader.images[control.backgroundName].bottomOffset || 0
        border.left: ConfigReader.images[control.backgroundName].leftOffset || 0
        border.right: ConfigReader.images[control.backgroundName].rightOffset || 0

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
