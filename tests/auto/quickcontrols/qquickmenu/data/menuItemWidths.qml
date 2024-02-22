// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 600
    height: 600

    property alias menu: menu

    Menu {
        id: menu
        MenuItem {
            objectName: "MenuItem"
            text: "MenuItem"
        }
        MenuSeparator {
            objectName: "MenuSeparator"
        }
        Menu {
            title: "Sub-menu"
            objectName: "Sub-menu"

            MenuItem {
                objectName: "SubMenuItem"
                text: "SubMenuItem"
            }
        }
        Rectangle {
            objectName: "CustomSeparator"
            height: 2
            color: "salmon"
        }
        Rectangle {
            // Use a binding to test retranslate(), which re-evaluates all bindings.
            implicitWidth: someValue
            objectName: "CustomRectangleSeparator"
            height: 2
            color: "salmon"

            property int someValue: 120
        }
        Control {
            objectName: "CustomControlSeparator"
            implicitWidth: someOtherValue
            height: 2
            background: Rectangle {
                color: "navajowhite"
            }

            property int someOtherValue: 180
        }
    }
}
