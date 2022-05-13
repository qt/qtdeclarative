// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.11


Item {
    visible: true
    Text {
        anchors.centerIn: parent
        text: "bla"
        MouseArea {
            anchors.fill: parent
        }
    }

    Timer {
        interval: 100;
        running: true;
        onTriggered: {
            Qt.quit();
        }
    }
}
