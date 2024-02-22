// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Item {
    width: 100; height: 100

    Loader {
       id: myLoader
       source: "MyItem.qml"
    }

    Connections {
        target: myLoader.item
        function onMessage(msg) { console.log(msg) }
    }
}
//![0]
