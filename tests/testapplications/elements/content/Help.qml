// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Item {
    anchors.fill: parent

    Text { textFormat: Text.RichText; anchors.fill: parent; anchors.margins: 5; wrapMode: Text.WordWrap
        text: "<b>Qt Quick 2 System Testing</b><br><br>"+
        "Each system test qml \"application\" provides a basic visual element affected by one or more non-visual (functional) elements.<br>"+
        "Simply select the element you wish to test, and follow the instructions. Use the arrow to advance the test.<br>"
    }
}
