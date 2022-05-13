// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Controls

ApplicationWindow {
    property alias childControl: childControl
    property alias childItem: childItem
    property alias childObject: childObject

    Control {
        id: childControl

        property ApplicationWindow attached_window: ApplicationWindow.window
        property Item attached_contentItem: ApplicationWindow.contentItem
        property Item attached_activeFocusControl: ApplicationWindow.activeFocusControl
        property Item attached_header: ApplicationWindow.header
        property Item attached_footer: ApplicationWindow.footer
    }

    Item {
        id: childItem

        property ApplicationWindow attached_window: ApplicationWindow.window
        property Item attached_contentItem: ApplicationWindow.contentItem
        property Item attached_activeFocusControl: ApplicationWindow.activeFocusControl
        property Item attached_header: ApplicationWindow.header
        property Item attached_footer: ApplicationWindow.footer
    }

    QtObject {
        id: childObject

        property ApplicationWindow attached_window: ApplicationWindow.window
        property Item attached_contentItem: ApplicationWindow.contentItem
        property Item attached_activeFocusControl: ApplicationWindow.activeFocusControl
        property Item attached_header: ApplicationWindow.header
        property Item attached_footer: ApplicationWindow.footer
    }

    property alias childWindow: childWindow
    property alias childWindowControl: childWindowControl
    property alias childWindowItem: childWindowItem
    property alias childWindowObject: childWindowObject

    Window {
        id: childWindow

        property ApplicationWindow attached_window: ApplicationWindow.window
        property Item attached_contentItem: ApplicationWindow.contentItem
        property Item attached_activeFocusControl: ApplicationWindow.activeFocusControl
        property Item attached_header: ApplicationWindow.header
        property Item attached_footer: ApplicationWindow.footer

        Control {
            id: childWindowControl

            property ApplicationWindow attached_window: ApplicationWindow.window
            property Item attached_contentItem: ApplicationWindow.contentItem
            property Item attached_activeFocusControl: ApplicationWindow.activeFocusControl
            property Item attached_header: ApplicationWindow.header
            property Item attached_footer: ApplicationWindow.footer
        }

        Item {
            id: childWindowItem

            property ApplicationWindow attached_window: ApplicationWindow.window
            property Item attached_contentItem: ApplicationWindow.contentItem
            property Item attached_activeFocusControl: ApplicationWindow.activeFocusControl
            property Item attached_header: ApplicationWindow.header
            property Item attached_footer: ApplicationWindow.footer
        }

        QtObject {
            id: childWindowObject

            property ApplicationWindow attached_window: ApplicationWindow.window
            property Item attached_contentItem: ApplicationWindow.contentItem
            property Item attached_activeFocusControl: ApplicationWindow.activeFocusControl
            property Item attached_header: ApplicationWindow.header
            property Item attached_footer: ApplicationWindow.footer
        }
    }

    property alias childAppWindow: childAppWindow
    property alias childAppWindowControl: childAppWindowControl
    property alias childAppWindowItem: childAppWindowItem
    property alias childAppWindowObject: childAppWindowObject

    ApplicationWindow {
        id: childAppWindow

        property ApplicationWindow attached_window: ApplicationWindow.window
        property Item attached_contentItem: ApplicationWindow.contentItem
        property Item attached_activeFocusControl: ApplicationWindow.activeFocusControl
        property Item attached_header: ApplicationWindow.header
        property Item attached_footer: ApplicationWindow.footer

        Control {
            id: childAppWindowControl

            property ApplicationWindow attached_window: ApplicationWindow.window
            property Item attached_contentItem: ApplicationWindow.contentItem
            property Item attached_activeFocusControl: ApplicationWindow.activeFocusControl
            property Item attached_header: ApplicationWindow.header
            property Item attached_footer: ApplicationWindow.footer
        }

        Item {
            id: childAppWindowItem

            property ApplicationWindow attached_window: ApplicationWindow.window
            property Item attached_contentItem: ApplicationWindow.contentItem
            property Item attached_activeFocusControl: ApplicationWindow.activeFocusControl
            property Item attached_header: ApplicationWindow.header
            property Item attached_footer: ApplicationWindow.footer
        }

        QtObject {
            id: childAppWindowObject

            property ApplicationWindow attached_window: ApplicationWindow.window
            property Item attached_contentItem: ApplicationWindow.contentItem
            property Item attached_activeFocusControl: ApplicationWindow.activeFocusControl
            property Item attached_header: ApplicationWindow.header
            property Item attached_footer: ApplicationWindow.footer
        }
    }
}
