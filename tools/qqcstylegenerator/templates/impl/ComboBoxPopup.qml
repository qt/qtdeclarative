import QtQuick
import QtQuick.FigmaStyle
import QtQuick.Templates as T

Popup {
    id: popup

    required property T.ComboBox __combobox

    topPadding: config.topPadding || 0
    leftPadding: config.leftPadding || 0
    rightPadding: config.rightPadding || 0
    bottomPadding: config.bottomPadding || 0

    topInset: -config.topInset || 0
    bottomInset: -config.bottomInset || 0
    leftInset: -config.leftInset || 0
    rightInset: -config.rightInset || 0

    readonly property string __currentState: [
        !__combobox.enabled && "disabled",
        __combobox.enabled && !__combobox.pressed && __combobox.hovered && "hovered",
        popup.visible && "open",
        __combobox.pressed && "pressed"
    ].filter(Boolean).join("-") || "normal"
    readonly property var config: ConfigReader.controls.comboboxpopup[__currentState] || {}

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
