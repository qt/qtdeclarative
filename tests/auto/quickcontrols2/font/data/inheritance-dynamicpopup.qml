// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window

    property Popup control
    property Control child
    property Control grandChild

    Component {
        id: popupComponent
        Popup { }
    }

    Component {
        id: controlComponent
        Control { }
    }

    Component.onCompleted: {
        control = popupComponent.createObject(window)
        child = controlComponent.createObject(control.contentItem)
        grandChild = controlComponent.createObject(child)
    }
}
