// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.iOS.impl

T.ComboBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    readonly property bool popupButtonStyle: control.flat && !control.editable

    spacing: popupButtonStyle ? 6 : 12
    padding: popupButtonStyle ? 0 : 12
    topPadding: 0
    bottomPadding: 0
    leftPadding: padding + (!control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width + spacing)
    rightPadding: padding + (control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width + spacing)

    delegate: MenuItem {
        width: ListView.view.width
        text: model[control.textRole]
        palette.text: control.palette.text
        palette.highlightedText: control.palette.highlightedText
        hoverEnabled: control.hoverEnabled
        checked: control.currentIndex === index

        required property var model
        required property int index

        readonly property bool isSingleItem: control.count === 1
        readonly property bool isFirstItem: !isSingleItem && index === 0
        readonly property bool isLastItem: !isSingleItem && index === control.count - 1

        background: Item {
            implicitHeight: 44
            NinePatchImage {
                y: isLastItem ? -1 : 0
                width: parent.width
                height: isLastItem ? parent.height + 1 : parent.height
                rotation: isLastItem ? 180 : 0
                visible: !(isSingleItem && !control.delegate.pressed)
                source: IOS.url + "menuitem-background"
                NinePatchImageSelector on source {
                    states: [
                        {"edge": isFirstItem || isLastItem },
                        {"single": isSingleItem},
                        {"light": Qt.styleHints.colorScheme === Qt.Light},
                        {"dark": Qt.styleHints.colorScheme === Qt.Dark},
                        {"pressed": down}
                    ]
                }
            }
        }
    }

    indicator: ColorImage {
        x: control.mirrored ? control.padding : control.width - width - control.padding
        y: control.topPadding + (control.availableHeight - height) / 2
        rotation: control.popupButtonStyle ? 0 : 90
        opacity: (control.enabled || control.popupButtonStyle) ? 1 : 0.5

        source: IOS.url + (control.popupButtonStyle ? "arrow-updown-indicator" : "arrow-indicator")
        color: control.popupButtonStyle ? (control.down ? control.palette.highlight : control.palette.button)
                                        : defaultColor
        ImageSelector on source {
            states: [
                {"light": Qt.styleHints.colorScheme === Qt.Light},
                {"dark": Qt.styleHints.colorScheme === Qt.Dark},
                {"pressed": control.down}
            ]
        }
    }

    contentItem: T.TextField {
        implicitWidth: control.popupButtonStyle ? Math.max(implicitBackgroundWidth + leftInset + rightInset,
                                contentWidth + leftPadding + rightPadding) : 0
        implicitHeight: control.popupButtonStyle ? Math.max(implicitBackgroundHeight + topInset + bottomInset,
                                 contentHeight + topPadding + bottomPadding) : 0
        padding: control.popupButtonStyle ? 0 : 6
        text: control.editable ? control.editText : control.displayText
        enabled: control.editable
        autoScroll: control.editable
        readOnly: control.down
        inputMethodHints: control.inputMethodHints
        validator: control.validator
        selectByMouse: control.selectTextByMouse

        color: control.popupButtonStyle ? (control.down ? control.palette.highlight : control.palette.button)
                                        : control.palette.text
        selectionColor: control.palette.highlight
        selectedTextColor: control.palette.highlightedText
        verticalAlignment: Text.AlignVCenter

        cursorDelegate: CursorDelegate {}
    }

    background: Rectangle {
        implicitWidth: control.popupButtonStyle ? implicitContentWidth : 250
        implicitHeight: 34
        radius: 4
        color: control.palette.base
        visible: !control.popupButtonStyle
    }

    popup: T.Popup {
        x: (control.width - width) / 2
        y: control.height + 6

        height: Math.min(contentItem.implicitHeight, control.Window.height - topMargin - bottomMargin)
        width: control.popupButtonStyle ? 250 : control.width

        topMargin: 6
        bottomMargin: 6

        enter: Transition {
            NumberAnimation { property: "scale"; from: 0.2; to: 1.0; easing.type: Easing.OutQuint; duration: 220 }
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutCubic; duration: 150 }
        }

        exit: Transition {
            NumberAnimation { property: "scale"; from: 1.0; to: 0.0; easing.type: Easing.OutQuint; duration: 220 }
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutCubic; duration: 150 }
        }
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.delegateModel
            currentIndex: control.highlightedIndex
            highlightMoveDuration: 0
        }
    }
}
