// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.iOS.impl

T.SearchField {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             searchIndicator.implicitIndicatorHeight + topPadding + bottomPadding)

    leftPadding: padding + (control.mirrored || !searchIndicator.indicator || !searchIndicator.indicator.visible ? 0 : searchIndicator.indicator.width + spacing)
    rightPadding: padding + (control.mirrored || !clearIndicator.indicator || !clearIndicator.indicator.visible ? 0 : clearIndicator.indicator.width + spacing)

    delegate: MenuItem {
        id: menuItem

        width: ListView.view.width
        text: model[control.textRole]
        palette.text: control.palette.text
        palette.highlightedText: control.palette.highlightedText
        font.weight: control.currentIndex === index ? Font.DemiBold : Font.Normal
        highlighted: control.currentIndex === index
        hoverEnabled: control.hoverEnabled
        checked: control.currentIndex === index

        required property var model
        required property int index

        readonly property bool isSingleItem: control.suggestionCount === 1
        readonly property bool isFirstItem: !isSingleItem && index === 0
        readonly property bool isLastItem: !isSingleItem && index === control.suggestionCount - 1

        background: Item {
            implicitHeight: 44
            NinePatchImage {
                y: menuItem.isLastItem ? -1 : 0
                width: parent.width
                height: menuItem.isLastItem ? parent.height + 1 : parent.height
                rotation: menuItem.isLastItem ? 180 : 0
                visible: !(menuItem.isSingleItem && !control.delegate.pressed)
                source: IOS.url + "menuitem-background"
                NinePatchImageSelector on source {
                    states: [
                        {"edge": menuItem.isFirstItem || menuItem.isLastItem},
                        {"single": menuItem.isSingleItem},
                        {"light": Application.styleHints.colorScheme === Qt.Light},
                        {"dark": Application.styleHints.colorScheme === Qt.Dark},
                        {"pressed": menuItem.down}
                    ]
                }
            }
        }
    }

    searchIndicator.indicator: IconLabel {
        x: !control.mirrored ? 6 : control.width - width - 6
        y: control.topPadding + (control.availableHeight - height) / 2

        icon.name: "system-search"
        icon.color: (Qt.styleHints.colorScheme === Qt.Dark ? Qt.lighter(control.palette.dark, 1.2) : Qt.darker(control.palette.dark, 1.2))
        icon.width: 16
        icon.height: 16
    }

    clearIndicator.indicator: IconLabel {
        x: control.mirrored ? 6 : control.width - width - 6
        y: control.topPadding + (control.availableHeight - height) / 2
        visible: control.contentItem.text

        icon.name: "edit-clear"
        icon.color: (Qt.styleHints.colorScheme === Qt.Dark ? Qt.lighter(control.palette.dark, 1.2) : Qt.darker(control.palette.dark, 1.2))
        icon.width: 17
        icon.height: 17
    }

    contentItem: T.TextField {
        implicitHeight: Math.max(contentHeight + topPadding + bottomPadding,
                                 placeholder.implicitHeight + topPadding + bottomPadding)

        leftPadding: control.searchIndicator.indicator && !control.mirrored ? 12 : 6
        rightPadding: control.clearIndicator.indicator && !control.mirrored ? 12 : 6
        topPadding: 6 - control.padding
        bottomPadding: 6 - control.padding

        text: control.text
        placeholderText: control.placeholderText

        PlaceholderText {
            id: placeholder
            x: parent.leftPadding
            y: parent.topPadding
            width: parent.width - parent.leftPadding - parent.rightPadding
            height: parent.height - parent.topPadding - parent.bottomPadding

            text: control.placeholderText
            font: parent.font
            color: control.palette.placeholderText
            visible: !parent.length && !parent.preeditText && (!parent.activeFocus || parent.horizontalAlignment !== Qt.AlignHCenter)
            verticalAlignment: parent.verticalAlignment
            elide: Text.ElideRight
            renderType: parent.renderType
        }

        selectByMouse: control.selectTextByMouse

        color: control.palette.text
        selectionColor: control.palette.highlight
        selectedTextColor: control.palette.highlightedText
        verticalAlignment: TextInput.AlignVCenter

        cursorDelegate: CursorDelegate {}

        ContextMenu.menu: TextEditingContextMenu {
            editor: parent
        }
    }

    background: Rectangle {
        implicitWidth: 150
        implicitHeight: 34
        radius: 8

        color: Qt.styleHints.colorScheme === Qt.Dark ? control.palette.base : Qt.lighter(control.palette.mid, 1.2)
        border.width: 0.5
        border.color: control.palette.mid
    }

    popup: null
}
