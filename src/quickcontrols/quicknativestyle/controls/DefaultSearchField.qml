// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Templates as T
import QtQuick.NativeStyle as NativeStyle

T.SearchField {
    id: control

    readonly property bool __nativeBackground: background instanceof NativeStyle.StyleItem
    readonly property bool __notCustomizable: true

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding,
                            90 /* minimum */ )
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             searchIndicator.implicitIndicatorHeight + topPadding + bottomPadding,
                             clearIndicator.implicitIndicatorHeight + topPadding + bottomPadding)

    leftPadding: (__nativeBackground ? background.contentPadding.left : 5)
    rightPadding: (__nativeBackground ? background.contentPadding.right : 5)
    topPadding: (__nativeBackground ? background.contentPadding.top : 2)
    bottomPadding: (__nativeBackground ? background.contentPadding.bottom : 2)

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

    contentItem: T.TextField {
        topPadding: 6 - control.padding
        bottomPadding: 6 - control.padding

        text: control.text
        placeholderText: control.placeholderText

        PlaceholderText {
            x: parent.leftPadding
            y: parent.topPadding
            width: parent.width - parent.leftPadding - parent.rightPadding
            height: parent.height - parent.topPadding - parent.bottomPadding

            text: control.placeholderText
            font: parent.font
            color: control.palette.placeholderText
            visible: !parent.length && !parent.preeditText && (!parent.activeFocus || parent.horizontalAlignment !== Qt.AlignHCenter)
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            renderType: parent.renderType
        }

        selectByMouse: control.selectTextByMouse

        font: control.font
        color: control.palette.text
        selectionColor: control.palette.highlight
        selectedTextColor: control.palette.highlightedText
        verticalAlignment: Text.AlignVCenter

        readonly property bool __ignoreNotCustomizable: true
    }

    searchIndicator.indicator: NativeStyle.SearchField {
        control: control
        subControl: NativeStyle.SearchField.Search
        y: control.topPadding + (control.availableHeight - height) / 2
        useNinePatchImage: false

        readonly property bool __ignoreNotCustomizable: true
    }

    clearIndicator.indicator: NativeStyle.SearchField {
        control: control
        subControl: NativeStyle.SearchField.Clear
        x: control.width - width - 5
        y: control.topPadding + (control.availableHeight - height) / 2
        useNinePatchImage: false

        readonly property bool __ignoreNotCustomizable: true
    }

    background: NativeStyle.SearchField {
        control: control
        subControl: NativeStyle.SearchField.Frame
        contentWidth: contentItem.implicitWidth
        contentHeight: contentItem.implicitHeight
        useNinePatchImage: false

        readonly property bool __ignoreNotCustomizable: true
    }

    popup: T.Popup {
        readonly property var layoutMargins: control.__nativeBackground ? control.background.layoutMargins : null
        x: layoutMargins ? layoutMargins.left : 0
        y: control.height - (layoutMargins ? layoutMargins.bottom : 0)
        width: control.width - (layoutMargins ? layoutMargins.left + layoutMargins.right : 0)
        height: Math.min(contentItem.implicitHeight, control.Window.height - topMargin - bottomMargin)
        topMargin: 6
        bottomMargin: 6

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
