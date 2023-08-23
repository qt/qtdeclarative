import QtQuick
import QtQuick.FigmaStyle
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.RangeSlider {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            first.implicitHandleWidth + leftPadding + rightPadding,
                            second.implicitHandleWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             first.implicitHandleHeight + topPadding + bottomPadding,
                             second.implicitHandleHeight + topPadding + bottomPadding)

    topPadding: horizontal ? config.topPadding : config.leftPadding || 0
    leftPadding: horizontal ? config.leftPadding : config.bottomPadding || 0
    rightPadding: horizontal ? config.rightPadding : config.topPadding || 0
    bottomPadding: horizontal ? config.bottomPadding : config.rightPadding || 0

    property string __controlState: [
        visualFocus && "focused",
        !control.enabled && "disabled",
        control.hovered && !(first.pressed || second.pressed) && "hovered",
    ].filter(Boolean).join("-") || "normal"
    readonly property var config: ConfigReader.controls.rangeslider[__controlState] || {}

    property string __firstHandleState: [
        first.hovered && !first.pressed && "hovered",
        first.pressed && "handle-pressed",
        visualFocus && "focused",
    ].filter(Boolean).join("-") || "normal"
    readonly property var firstHandleConfig: ConfigReader.controls.rangeslider[__firstHandleState] || {}

    property string __secondHandleState: [
        second.hovered && !second.pressed && "hovered",
        second.pressed && "handle-pressed",
        visualFocus && "focused",
    ].filter(Boolean).join("-") || "normal"
    readonly property var secondHandleConfig: ConfigReader.controls.rangeslider[__secondHandleState] || {}

    first.handle: StyleImage {
        x: Math.round(control.leftPadding + (control.horizontal
            ? control.first.visualPosition * (control.availableWidth - width)
            : (control.availableWidth - width) / 2))
        y: Math.round(control.topPadding + (control.horizontal
            ? (control.availableHeight - height) / 2
            : control.first.visualPosition * (control.availableHeight - height)))

        imageConfig: control.firstHandleConfig.first_handle
    }

    second.handle: StyleImage {
        x: Math.round(control.leftPadding + (control.horizontal
            ? control.second.visualPosition * (control.availableWidth - width)
            : (control.availableWidth - width) / 2))
        y: Math.round(control.topPadding + (control.horizontal
            ? (control.availableHeight - height) / 2
            : control.second.visualPosition * (control.availableHeight - height)))

        imageConfig: control.secondHandleConfig.second_handle
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
                    ? control.first.handle.width / 2
                    : (control.availableWidth - width) / 2)
                y: control.topPadding - control.rightInset + (control.horizontal
                    ? ((control.availableHeight - height) / 2)
                    : control.first.handle.height / 2)

                width: control.horizontal
                    ? control.availableWidth
                        - (control.first.handle.width / 2) - (control.second.handle.width / 2)
                    : implicitWidth
                height: control.horizontal
                    ? implicitHeight
                    : control.availableHeight
                        - (control.first.handle.width / 2) - (control.second.handle.width / 2)
                imageConfig: control.config.groove
                horizontal: control.horizontal

                property Item track: StyleImage {
                    parent: control.background._background.groove
                    x: horizontal ? parent.width * control.first.position : 0
                    y: horizontal ? 0 : parent.height - (parent.height * control.second.position)
                    width: horizontal
                        ? parent.width * (control.second.position - control.first.position)
                        : parent.width
                    height: horizontal
                        ? parent.height
                        : parent.height * (control.second.position - control.first.position)
                    imageConfig: control.config.track
                    horizontal: control.horizontal
                }
            }
        }
    }
}
