// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window
import QtQuick.Controls.impl
import QtQuick.Templates as T
import QtQuick.Controls.Material
import QtQuick.Controls.Material.impl

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

    delegate: MenuItem {
        id: delegate

        width: ListView.view.width
        highlighted: control.highlightedIndex === index
        hoverEnabled: control.hoverEnabled

        Material.foreground: control.currentIndex === index ? ListView.view.contentItem.Material.accent : ListView.view.contentItem.Material.foreground

        contentItem: Text {
            text: delegate.model[control.textRole]
            color: delegate.enabled ? delegate.Material.foreground : delegate.Material.hintTextColor
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        required property var model
        required property int index
    }

    searchIndicator.indicator: Item {
        x: !control.mirrored ? 10 : control.width - width - 10
        y: control.topPadding
        height: control.availableHeight
        width: height / 2

        ColorImage {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2

            source: "qrc:/qt-project.org/imports/QtQuick/Controls/Material/images/search-magnifier.png"
            color: control.enabled ? control.Material.foreground : control.Material.hintTextColor
        }
    }

    clearIndicator.indicator: Item {
        x: control.mirrored ? 10 : control.width - width - 10
        y: control.topPadding
        height: control.availableHeight
        width: height / 2
        visible: control.text.length > 0

        ColorImage {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2

            source: "qrc:/qt-project.org/imports/QtQuick/Controls/Material/images/close_circle.png"
            color: control.enabled ? control.Material.foreground : control.Material.hintTextColor
        }
    }

    contentItem: T.TextField {
        implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                                 contentHeight + topPadding + bottomPadding)

        leftPadding: Material.textFieldHorizontalPadding
                     + (control.mirrored ? (control.__clearIndicatorVisible ? 6 : 3)
                                         : (control.__searchIndicatorVisible ? 6 : 3))
        rightPadding: Material.textFieldHorizontalPadding
                      + (control.mirrored ? (control.__searchIndicatorVisible ? 6 : 3)
                                          : (control.__clearIndicatorVisible ? 6 : 3))
        topPadding: Material.textFieldVerticalPadding
        bottomPadding: Material.textFieldVerticalPadding

        // If we're clipped, set topInset to half the height of the placeholder text to avoid it being clipped.
        topInset: clip ? placeholder.height / 2 : 0

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
            color: parent.enabled && parent.activeFocus ? Material.accentColor : Material.hintTextColor
            visible: !parent.length && !parent.preeditText && (!parent.activeFocus || parent.horizontalAlignment !== Qt.AlignHCenter)
            verticalAlignment: parent.verticalAlignment
            elide: Text.ElideRight
            renderType: parent.renderType
        }

        selectByMouse: control.selectTextByMouse

        color: control.enabled ? control.Material.foreground : control.Material.hintTextColor
        selectionColor: control.Material.accentColor
        selectedTextColor: control.Material.primaryHighlightedTextColor
        verticalAlignment: Text.AlignVCenter

        ContextMenu.menu: TextEditingContextMenu {
            editor: parent
        }

        cursorDelegate: CursorDelegate { }
    }

    background: MaterialTextContainer {
        implicitWidth: 160
        implicitHeight: control.Material.textFieldHeight

        outlineColor: (enabled && control.hovered) ? control.Material.primaryTextColor : control.Material.hintTextColor
        focusedOutlineColor: control.Material.accentColor
        controlHasActiveFocus: control.activeFocus
        controlHasText: true
        horizontalPadding: control.Material.textFieldHorizontalPadding
    }

    popup: T.Popup {
        y: control.height
        width: control.width
        height: contentItem.implicitHeight > 0 ? Math.min(contentItem.implicitHeight + verticalPadding * 2, control.Window.height - control.y - control.height - control.padding) : 0
        topMargin: 10
        bottomMargin: 10
        verticalPadding: 10

        Material.theme: control.Material.theme
        Material.accent: control.Material.accent
        Material.primary: control.Material.primary

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.delegateModel
            currentIndex: control.highlightedIndex
            highlightMoveDuration: 0

            T.ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            radius: 5
            color: control.Material.dialogColor

            layer.enabled: control.enabled > 0
            layer.effect: RoundedElevationEffect {
                elevation: 4
                roundedScale: Material.ExtraSmallScale
            }
        }
    }
}
