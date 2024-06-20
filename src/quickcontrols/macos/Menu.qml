// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Window
import QtQuick.Effects

T.Menu {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    // The insets are found by examining the MultiEffect.itemRect, which
    // contains the drop shadow offsets. QQuickPopup will subract these insets when
    // it opens up the menu so that the top left corner of the background ends up at
    // the requested popup position.
    // Note: the insets are hard-coded to avoid a binding loop to implicit size.
    leftInset: 32
    topInset: 32
    rightInset: 32
    bottomInset: 32
    leftPadding: leftInset + 5
    rightPadding: rightInset + 5
    topPadding: topInset + 5
    bottomPadding: bottomInset + 5
    margins: 0
    overlap: 4

    delegate: MenuItem { }

    contentItem: ListView {
        implicitHeight: contentHeight
        model: control.contentModel
        interactive: Window.window
                     ? contentHeight + control.topPadding + control.bottomPadding > control.height
                     : false
        currentIndex: control.currentIndex
        spacing: 2

        ScrollIndicator.vertical: ScrollIndicator {}
    }

    background: MultiEffect {
        implicitWidth: 200
        implicitHeight: 20
        source: Rectangle {
            width: control.background.width
            height: control.background.height
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
        shadowScale: 1.04
        shadowOpacity: Application.styleHints.colorScheme === Qt.Light ? 0.15 : 0.2
        shadowColor: 'black'
        shadowEnabled: true
        shadowHorizontalOffset: 0
        shadowVerticalOffset: 7
    }

    T.Overlay.modal: Rectangle {
        color: "transparent"
    }

    T.Overlay.modeless: Rectangle {
        color: "transparent"
    }
}
