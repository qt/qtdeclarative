// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window

    property Popup control
    property Control child
    property Control grandChild

    Item {
        id: childItem
    }

    Component {
        id: popupComponent
        Popup { }
    }

    Component {
        id: controlComponent
        Control { }
    }

    Component.onCompleted: {
        control = popupComponent.createObject(childItem)
        child = controlComponent.createObject(control.contentItem)
        grandChild = controlComponent.createObject(child)
    }
}
