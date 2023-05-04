import QtQuick
import QtQuick.FigmaStyle
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.TextField {
    id: control

    implicitWidth: implicitBackgroundWidth + leftInset + rightInset
                   || Math.max(contentWidth, placeholder.implicitWidth) + leftPadding + rightPadding
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding,
                             placeholder.implicitHeight + topPadding + bottomPadding)

    readonly property var config: ConfigReader.configForImageUrl(background?.source ?? "")

    topPadding: config?.topPadding || 0
    bottomPadding: config?.bottomPadding || 0
    leftPadding: config?.leftPadding || 0
    rightPadding: config?.rightPadding || 0

    topInset: -config?.topInset || 0
    bottomInset: -config?.bottomInset || 0
    leftInset: -config?.leftInset || 0
    rightInset: -config?.rightInset || 0

    color: control.palette.text
    selectionColor: control.palette.highlight
    selectedTextColor: control.palette.highlightedText
    placeholderTextColor: control.palette.placeholderText
    verticalAlignment: Qt.AlignVCenter

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
        visible: !control.length && !control.preeditText && (!control.activeFocus || control.horizontalAlignment !== Qt.AlignHCenter)
        elide: Text.ElideRight
        renderType: control.renderType
    }

    background: BorderImage {
        source: Qt.resolvedUrl("images/textfield-background")

        border.top: control.config?.topOffset || 0
        border.bottom: control.config?.bottomOffset || 0
        border.left: control.config?.leftOffset || 0
        border.right: control.config?.rightOffset || 0

        ImageSelector on source {
            states: [
                {"disabled": !control.enabled},
                {"focused": control.activeFocus},
                {"hovered": control.enabled && control.hovered}
            ]
        }
    }
}
