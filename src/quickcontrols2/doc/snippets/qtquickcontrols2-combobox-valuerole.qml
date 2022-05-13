// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

//! [file]
ApplicationWindow {
    width: 640
    height: 480
    visible: true

    // Used as an example of a backend - this would usually be
    // e.g. a C++ type exposed to QML.
    QtObject {
        id: backend
        property int modifier
    }

    ComboBox {
        textRole: "text"
        valueRole: "value"
        // When an item is selected, update the backend.
        onActivated: backend.modifier = currentValue
        // Set the initial currentIndex to the value stored in the backend.
        Component.onCompleted: currentIndex = indexOfValue(backend.modifier)
        model: [
            { value: Qt.NoModifier, text: qsTr("No modifier") },
            { value: Qt.ShiftModifier, text: qsTr("Shift") },
            { value: Qt.ControlModifier, text: qsTr("Control") }
        ]
    }
}
//! [file]
