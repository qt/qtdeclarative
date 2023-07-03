// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
//![0]
import QtQuick

Rectangle {
    width: 200; height: 200

    Loader {
        id: loader
        focus: true
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            loader.source = "KeyReader.qml"
        }
    }

    Keys.onPressed: (event)=> {
        console.log("Captured:",
                    event.text);
    }
}
//![0]

