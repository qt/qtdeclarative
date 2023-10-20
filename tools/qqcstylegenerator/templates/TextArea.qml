import QtQuick
import QtQuick.FigmaStyle
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.TextArea {
    id: control

    implicitWidth: Math.max((background.minimumWidth || implicitBackgroundWidth)
                            + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max((background.minimumHeight || implicitBackgroundHeight)
                            + topInset + bottomInset,
                            contentHeight + topPadding + bottomPadding)

    topPadding: config.topPadding || 0
    bottomPadding: config.bottomPadding || 0
    leftPadding: config.leftPadding || 0
    rightPadding: config.rightPadding || 0

    topInset: -config.topInset || 0
    bottomInset: -config.bottomInset || 0
    leftInset: -config.leftInset || 0
    rightInset: -config.rightInset || 0

    color: control.palette.text
    selectionColor: control.palette.highlight
    selectedTextColor: control.palette.highlightedText
    verticalAlignment: control.config.label.textVAlignment
    horizontalAlignment: control.config.label.textHAlignment
    placeholderTextColor: control.palette.placeholderText

    readonly property string __currentState: [
        !enabled && "disabled",
        activeFocus && "focused",
        enabled && !activeFocus && hovered && "hovered",
    ].filter(Boolean).join("_") || "normal"
    readonly property var config: Config.controls.textarea[__currentState] || {}

    PlaceholderText {
        id: placeholder
        x: control.leftPadding
        y: control.topPadding
        width: control.width - (control.leftPadding + control.rightPadding)
        height: control.height - (control.topPadding + control.bottomPadding)

        text: control.placeholderText
        font: control.font
        color: control.placeholderTextColor
        verticalAlignment: control.verticalAlignment
        horizontalAlignment: control.horizontalAlignment
        visible: !control.length && !control.preeditText && (!control.activeFocus || control.horizontalAlignment !== Qt.AlignHCenter)
        elide: Text.ElideRight
        renderType: control.renderType
    }

    background: StyleImage {
        imageConfig: control.config.background
    }
}
