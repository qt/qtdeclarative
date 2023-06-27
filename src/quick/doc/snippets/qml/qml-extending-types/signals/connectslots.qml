// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
//![0]
Item {
    id: item
    width: 200; height: 200

    function myMethod() {
        console.log("Button was clicked!")
    }

    Button {
        id: button
        anchors.fill: parent
        Component.onCompleted: buttonClicked.connect(item.myMethod)
    }
}
//![0]
