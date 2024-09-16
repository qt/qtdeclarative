// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window

    property Control control
    property Control child
    property Control grandChild

    Item {
        id: childItem
    }

    Component {
        id: component
        Control { }
    }

    Component.onCompleted: {
        control = component.createObject(childItem)
        child = component.createObject(control)
        grandChild = component.createObject(child)
    }
}
