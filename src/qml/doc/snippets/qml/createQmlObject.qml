// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: parentItem

    width: 100
    height: 100

    function createIt() {
//![0]
const newObject = Qt.createQmlObject(`
    import QtQuick

    Rectangle {
        color: "red"
        width: 20
        height: 20
    }
    `,
    parentItem,
    "myDynamicSnippet"
);
//![0]

//![destroy]
newObject.destroy(1000);
//![destroy]
    }

    Component.onCompleted: createIt()
}
