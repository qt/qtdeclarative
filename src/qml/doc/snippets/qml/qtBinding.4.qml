// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//![0]
Item {
    width: 50
    property var storedBindings: [ Qt.binding(function() { return x + width }) ] // stored
    property int a: Qt.binding(function() { return x + width }) // error!
    property int b

    Component.onCompleted: {
        b = storedBindings[0] // causes binding assignment
    }
}
//![0]
