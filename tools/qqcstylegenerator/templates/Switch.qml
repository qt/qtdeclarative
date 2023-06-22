import QtQuick
import QtQuick.FigmaStyle
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.Switch {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding,
                            implicitIndicatorWidth)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    spacing: config.spacing || 0

    topPadding: config.topPadding || 0
    leftPadding: config.topPadding || 0
    rightPadding: config.rightPadding || 0
    bottomPadding: config.bottomPadding || 0

    topInset: -config.topInset || 0
    bottomInset: -config.bottomInset || 0
    leftInset: -config.leftInset || 0
    rightInset: -config.rightInset || 0

    readonly property string currentState: [
        control.checked && "checked",
        !control.enabled && "disabled",
        control.enabled && !control.down && control.hovered && "hovered",
        control.down && "pressed"
    ].filter(Boolean).join("-") || "normal"
    readonly property var config: ConfigReader.controls.switch[currentState] || {}
    readonly property bool mirroredIndicator: control.mirrored !== (config.mirrored || false)

    indicator: Item {
        x: control.mirroredIndicator ? control.width - width - control.rightPadding : control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        // If handleBackground is not generated, use the size of the handle
        implicitWidth: handleBackground.width > 0 ? handleBackground.width : handleBackground.handle.width * 2
        implicitHeight: handleBackground.height > 0 ? handleBackground.height : handleBackground.handle.height

        property Image handleBackground: Image {
            parent: control.indicator
            source: Qt.resolvedUrl(control.config.handle_background.filePath)

            property Image handle: Image {
                parent: control.indicator.handleBackground
                x: control.config.handle_contentItem.leftPadding
                    + (control.visualPosition * (parent.width - width
                        - control.config.handle_contentItem.leftPadding
                        - control.config.handle_contentItem.rightPadding))
                y: control.config.handle_contentItem.topPadding
                source: Qt.resolvedUrl(control.config.handle.filePath)

                Behavior on x {
                    enabled: !control.down
                    SmoothedAnimation {
                        velocity: 200
                    }
                }
            }
        }
    }

    contentItem: Text {
        leftPadding: control.indicator && !control.mirroredIndicator
            ? control.indicator.x + control.indicator.width + control.spacing
            : 0
        rightPadding: control.indicator && control.mirroredIndicator
            ? control.width - control.indicator.x + control.spacing
            : 0
        text: control.text
        font: control.font
        color: control.palette.windowText
        elide: Text.ElideRight
        horizontalAlignment: control.config.label.textHAlignment
        verticalAlignment: control.config.label.textVAlignment
    }

    background: BorderImage {
        source: Qt.resolvedUrl(control.config.background.filePath)
        border {
            top: control.config.background?.topOffset || 0
            bottom: control.config.background?.bottomOffset || 0
            left: control.config.background?.leftOffset || 0
            right: control.config.background?.rightOffset || 0
        }
    }
}
