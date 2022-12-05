// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window

    property Control control
    property Control child
    property Control grandChild

    Component {
        id: component
        Control { }
    }

    Component.onCompleted: {
        control = component.createObject(contentItem)
        child = component.createObject(control)
        grandChild = component.createObject(child)
    }
}
