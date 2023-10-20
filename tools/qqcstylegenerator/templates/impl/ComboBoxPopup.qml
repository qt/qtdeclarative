import QtQuick
import QtQuick.FigmaStyle
import QtQuick.Templates as T

T.Popup {
    id: popup

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    topPadding: config.topPadding || 0
    leftPadding: config.leftPadding || 0
    rightPadding: config.rightPadding || 0
    bottomPadding: config.bottomPadding || 0

    topInset: -config.topInset || 0
    bottomInset: -config.bottomInset || 0
    leftInset: -config.leftInset || 0
    rightInset: -config.rightInset || 0

    required property T.ComboBox __combobox

    readonly property string __currentState: [
        !__combobox.enabled && "disabled",
        __combobox.enabled && !__combobox.pressed && __combobox.hovered && "hovered",
        popup.visible && "open",
        __combobox.pressed && "pressed"
    ].filter(Boolean).join("_") || "normal"
    readonly property var config: Config.controls.comboboxpopup[__currentState] || {}

    contentItem: ListView {
        clip: true
        implicitHeight: contentHeight
        spacing: config.contentItem.spacing
        highlightMoveDuration: 0

        model: __combobox.delegateModel
        currentIndex: __combobox.highlightedIndex

        ScrollIndicator.vertical: ScrollIndicator { }
    }

    background: StyleImage {
        imageConfig: config.background
    }
}
