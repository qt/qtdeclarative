// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
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
