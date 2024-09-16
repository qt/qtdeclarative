// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtQuick.Window 2.0
import QtTest 1.1

Item {
    width: height
    height: 40
    MouseArea {
        id: ma
        anchors.fill: parent

        property bool everClicked: false

        onClicked: everClicked = true;
    }

    Window {
        id: ma2Window
        width: height
        height: 40
        MouseArea {
            id: ma2
            anchors.fill: parent

            property bool everClicked: false

            onClicked: everClicked = true;
        }

        Component.onCompleted: ma2Window.show();
    }

    TestCase {
        name: "ClickWindow"
        when: windowShown

        function test_clickBothWindows() {
            if (Qt.platform.os === "linux" || Qt.platform.os === "windows")
                skip("more details in QTBUG-53785")
            mouseClick(ma, 20, 20);
            verify(ma.everClicked);
            mouseClick(ma2, 20, 20);
            verify(ma2.everClicked);
        }
    }
}
