// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import QtQuick.Controls.Basic.impl

T.SearchField {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             searchIndicator.implicitIndicatorHeight + topPadding + bottomPadding,
                             clearIndicator.implicitIndicatorHeight + topPadding + bottomPadding)

    readonly property bool __searchIndicatorVisible: control.searchIndicator.indicator && control.searchIndicator.indicator.visible
    readonly property bool __clearIndicatorVisible: control.clearIndicator.indicator && control.clearIndicator.indicator.visible

    leftPadding: padding + (control.mirrored
                            ? (control.__clearIndicatorVisible ? control.clearIndicator.indicator.width + spacing : 0)
                            : (control.__searchIndicatorVisible ? control.searchIndicator.indicator.width + spacing : 0))

    rightPadding: padding + (control.mirrored
                             ? (control.__searchIndicatorVisible ? control.searchIndicator.indicator.width + spacing : 0)
                             : (control.__clearIndicatorVisible ? control.clearIndicator.indicator.width + spacing : 0))

    delegate: ItemDelegate {
        id: delegate

        width: ListView.view.width
        highlighted: control.highlightedIndex === index
        hoverEnabled: control.hoverEnabled

        contentItem: Text {
            text: delegate.model[control.textRole]
            font.weight: control.currentIndex === index ? Font.DemiBold : Font.Normal
            color: delegate.highlighted ? palette.highlightedText : palette.windowText
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        required property var model
        required property int index
    }

    searchIndicator.indicator: Rectangle {
        implicitWidth: 28
        implicitHeight: 28
        height: control.height - (background.border.width * 2)

        x: !control.mirrored ? 3 : control.width - width - 3
        y: background.border.width
        color: control.palette.button

        ColorImage {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            color: control.palette.dark
            defaultColor: "#353637"
            source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/search-magnifier.png"
            opacity: enabled ? 1 : 0.3
        }
    }

    clearIndicator.indicator: Rectangle {
        implicitWidth: 28
        implicitHeight: 28
        height: control.height - (background.border.width * 2)

        x: control.mirrored ? 3 : control.width - width - 3
        y: background.border.width
        visible: control.text.length > 0
        color: control.palette.button

        ColorImage {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            color: control.palette.dark
            defaultColor: "#353637"
            source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/close_circle.png"
            opacity: enabled ? 1 : 0.3
        }
    }

    contentItem: T.TextField {
        implicitHeight: Math.max(contentHeight + topPadding + bottomPadding,
                                 placeholder.implicitHeight + topPadding + bottomPadding)
        leftPadding: control.mirrored ? (control.__clearIndicatorVisible ? 6 : 3)
                                      : (control.__searchIndicatorVisible ? 6 : 3)
        rightPadding: control.mirrored ? (control.__searchIndicatorVisible ? 6 : 3)
                                       : (control.__clearIndicatorVisible ? 6 : 3)
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

        ContextMenu.menu: TextEditingContextMenu {
            editor: parent
        }
    }

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 40

        color: control.palette.button
        border.width: (control.activeFocus || control.contentItem.activeFocus) ? 2 : 1
        border.color: (control.activeFocus || control.contentItem.activeFocus) ? control.palette.highlight : control.palette.mid
    }

    popup: T.Popup {
        y: control.height
        width: control.width
        height: Math.min(contentItem.implicitHeight, control.Window.height - control.y - control.height - control.padding)
        topMargin: 6
        bottomMargin: 6
        palette: control.palette

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.delegateModel
            currentIndex: control.highlightedIndex
            highlightMoveDuration: 0

            Rectangle {
                z: 10
                width: parent.width
                height: parent.height
                color: "transparent"
                border.color: control.palette.mid
            }

            T.ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            color: control.palette.window
        }
    }
}
