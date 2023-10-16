import QtQuick
import QtQuick.FigmaStyle
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.Slider {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitHandleWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitHandleHeight + topPadding + bottomPadding)

    topPadding: horizontal ? config.topPadding : config.leftPadding || 0
    leftPadding: horizontal ? config.leftPadding : config.bottomPadding || 0
    rightPadding: horizontal ? config.rightPadding : config.topPadding || 0
    bottomPadding: horizontal ? config.bottomPadding : config.rightPadding || 0

    readonly property string currentState: [
        !control.enabled && "disabled",
        control.enabled && !control.pressed && control.hovered && "hovered",
        control.pressed && "pressed"
    ].filter(Boolean).join("-") || "normal"
    readonly property var config: ConfigReader.controls.slider[currentState] || {}

    handle: StyleImage {
        x: Math.round(control.leftPadding + (control.horizontal
            ? control.visualPosition * (control.availableWidth - width)
            : (control.availableWidth - width) / 2))
        y: Math.round(control.topPadding + (control.horizontal
            ? (control.availableHeight - height) / 2
            : control.visualPosition * (control.availableHeight - height)))

        imageConfig: control.config.handle
    }

    background: Item {
        implicitWidth: control.horizontal
            ? (_background.implicitWidth || _background.groove.implicitWidth)
            : (_background.implicitHeight || _background.groove.implicitHeight)
        implicitHeight: control.horizontal
            ? (_background.implicitHeight || _background.groove.implicitHeight)
            : (_background.implicitWidth || _background.groove.implicitWidth)

        property Item _background: StyleImage {
            parent: control.background
            anchors.fill: parent
            imageConfig: control.config.background

            property Item groove: StyleImage {
                parent: control.background._background
                x: control.leftPadding - control.leftInset + (control.horizontal
                    ? control.handle.width / 2
                    : (control.availableWidth - width) / 2)
                y: control.topPadding - control.topInset + (control.horizontal
                    ? ((control.availableHeight - height) / 2)
                    : control.handle.height / 2)

                width: control.horizontal
                    ? control.availableWidth - control.handle.width
                    : implicitWidth
                height: control.horizontal
                    ? implicitHeight
                    : control.availableHeight - control.handle.width
                imageConfig: control.config.groove
                horizontal: control.horizontal

                property Item track: StyleImage {
                    parent: control.background._background.groove
                    y: horizontal ? 0 : parent.height - (parent.height * control.position)
                    width: horizontal ? parent.width * control.position : parent.width
                    height: horizontal ? parent.height : parent.height * control.position
                    imageConfig: control.config.track
                    horizontal: control.horizontal
                    minimumWidth: 0
                    minimumHeight: 0
                }
            }
        }
    }
}
