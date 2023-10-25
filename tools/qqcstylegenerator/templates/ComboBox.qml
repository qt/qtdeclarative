pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.ComboBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    spacing: config.spacing || 0

    topPadding: config.topPadding || 0
    bottomPadding: config.bottomPadding || 0
    leftPadding: (config.leftPadding + (!control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width + spacing)) || 0
    rightPadding: (config.rightPadding + (control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width + spacing)) || 0

    topInset: -config.topInset || 0
    bottomInset: -config.bottomInset || 0
    leftInset: -config.leftInset || 0
    rightInset: -config.rightInset || 0

    readonly property string __currentState: [
        !control.enabled && "disabled",
        control.enabled && !control.pressed && control.hovered && "hovered",
        control.popup.visible && "open",
        control.pressed && "pressed"
    ].filter(Boolean).join("_") || "normal"
    readonly property var config: (control.editable ? Config.controls.editablecombobox[__currentState] : Config.controls.combobox[__currentState]) || {}


    delegate: ItemDelegate {
        required property var model
        required property int index

        width: ListView.view.width
        text: model[control.textRole]
        palette.text: control.palette.text
        palette.highlightedText: control.palette.highlightedText
        font.weight: control.currentIndex === index ? Font.DemiBold : Font.Normal
        highlighted: control.highlightedIndex === index
        hoverEnabled: control.hoverEnabled
    }

    indicator: Image {
        x: control.mirrored ? control.config.leftPadding : control.width - width - control.config.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        source: Qt.resolvedUrl(control.config.indicator.filePath)
    }

    contentItem: T.TextField {
        text: control.editable ? control.editText : control.displayText

        topPadding: control.config.label_contentItem.topPadding || 0
        leftPadding: control.config.label_contentItem.leftPadding || 0
        rightPadding: control.config.label_contentItem.rightPadding || 0
        bottomPadding: control.config.label_contentItem.bottomPadding || 0

        implicitWidth: (implicitBackgroundWidth + leftInset + rightInset)
                        || contentWidth + leftPadding + rightPadding
        implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                                 contentHeight + topPadding + bottomPadding)

        enabled: control.editable
        autoScroll: control.editable
        readOnly: control.down
        inputMethodHints: control.inputMethodHints
        validator: control.validator
        selectByMouse: control.selectTextByMouse

        color: control.palette.text
        selectionColor: control.palette.highlight
        selectedTextColor: control.palette.highlightedText
        horizontalAlignment: control.config.label_text.textHAlignment
        verticalAlignment: control.config.label_text.textVAlignment

        background: StyleImage {
            imageConfig: control.config.label_background
        }
    }

    background: StyleImage {
        imageConfig: control.config.background
    }

    popup: ComboBoxPopup {
        __combobox: control
        y: control.height
        height: Math.min(contentItem.implicitHeight + topPadding + bottomPadding, control.Window.height - topMargin - bottomMargin)
        width: control.width
    }
}
