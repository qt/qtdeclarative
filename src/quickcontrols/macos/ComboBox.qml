// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Templates as T
import QtQuick.NativeStyle as NativeStyle
import QtQuick.Controls.macOS.impl
import QtQuick.Effects

NativeStyle.DefaultComboBox {
    id: control
    readonly property Item __focusFrameTarget: control

    background: NativeStyle.ComboBox {
        control: control
        contentWidth: contentItem.implicitWidth
        contentHeight: contentItem.implicitHeight
        useNinePatchImage: false

        readonly property bool __ignoreNotCustomizable: true
    }

    contentItem: T.TextField {
        implicitWidth: contentWidth
        implicitHeight: contentHeight
        text: control.editable ? control.editText : control.displayText

        enabled: control.editable
        autoScroll: control.editable
        readOnly: control.down
        inputMethodHints: control.inputMethodHints
        validator: control.validator
        selectByMouse: control.selectTextByMouse

        color: control.editable ? control.palette.text : control.palette.buttonText
        selectionColor: control.palette.highlight
        selectedTextColor: control.palette.highlightedText
        verticalAlignment: Text.AlignVCenter

        readonly property Item __focusFrameControl: control
        readonly property bool __ignoreNotCustomizable: true

        ContextMenu.menu: TextEditingContextMenu {
            editor: parent
        }
    }

    delegate: MenuItem {
        required property var model
        required property int index

        text: model[control.textRole]
        highlighted: control.highlightedIndex === index
        hoverEnabled: control.hoverEnabled
        background.implicitWidth: ListView.view.width
    }

    popup: T.Popup {
        readonly property var layoutMargins: control.__nativeBackground ? control.background.layoutMargins : null
        readonly property real __maxHeight: popupType === Popup.Window ? Screen.height - SafeArea.margins.top - SafeArea.margins.bottom
                                                                       : control.Window.height - topMargin - bottomMargin
        x: layoutMargins ? layoutMargins.left : 0
        y: control.height - (layoutMargins ? layoutMargins.bottom : 0)
        popupType: Popup.Window
        width: Math.max(control.width, implicitContentWidth + leftPadding + rightPadding,implicitBackgroundWidth + leftInset + rightInset)
        height: Math.min(Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding), __maxHeight)
        margins: 0
        leftPadding: 5
        topPadding: 5
        rightPadding: 5
        bottomPadding: 5
        leftInset: -32
        topInset: -32
        rightInset: -32
        bottomInset: -32
        palette: control.palette
        contentItem: ListView {
            clip: true
            model: control.delegateModel
            currentIndex: control.highlightedIndex
            implicitHeight: contentHeight
            spacing: 2
            highlightMoveDuration: 0
            T.ScrollIndicator.vertical: ScrollIndicator { }
        }
        background: Item {
            implicitWidth: 100 - control.popup.leftInset - control.popup.rightInset
            implicitHeight: 0 - control.popup.topInset - control.popup.bottomInset
            MultiEffect {
                x: -control.popup.leftInset
                y: -control.popup.topInset
                width: source.width
                height: source.height
                source: Rectangle {
                    width: control.popup.background.width + control.popup.leftInset + control.popup.rightInset
                    height: control.popup.background.height + control.popup.topInset + control.popup.bottomInset
                    radius: 5
                    color: Application.styleHints.colorScheme === Qt.Light
                           ? Qt.darker(control.palette.window, 1.04)
                           : Qt.darker(control.palette.window, 1.2)
                    border.color: Application.styleHints.colorScheme === Qt.Light
                                  ? Qt.darker(control.palette.window, 1.4)
                                  : Qt.lighter(control.palette.window, 2.0)
                    border.width: 0.5
                    visible: false
                }
                shadowScale: 1
                shadowOpacity: Application.styleHints.colorScheme === Qt.Light ? 0.15 : 0.2
                shadowColor: 'black'
                shadowEnabled: true
                shadowHorizontalOffset: 0
                shadowVerticalOffset: 6
            }
        }
    }
}
