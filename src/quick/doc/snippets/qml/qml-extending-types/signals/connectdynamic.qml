// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
//![0]
Item {
    id: item
    width: 300; height: 100

    function myMethod() {
        console.log("Button was clicked!")
    }

    Row { id: row }

    Component.onCompleted: {
        var component = Qt.createComponent("Button.qml")
        for (var i=0; i<3; i++) {
            var button = component.createObject(row)
            button.border.width = 1
            button.buttonClicked.connect(myMethod)
        }
    }
}
//![0]
