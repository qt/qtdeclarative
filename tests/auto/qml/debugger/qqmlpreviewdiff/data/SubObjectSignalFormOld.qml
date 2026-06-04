// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Mimics CoffeeCardForm.ui.qml: a component with a child C++ object that has
// a signal (Timer.triggered), exposed to the outside via an alias.
// This is the inner-most type whose CU gets modified during hot-reload.

import QtQml
import QtQuick

Item {
    id: root
    property alias button: button

    Timer {
        id: button
        interval: 1000
    }

    Rectangle {
        id: indicator
        width: 50
        height: 50
        color: "red"
    }
}
