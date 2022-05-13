// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Image {
    property bool explicitSize: true
    property alias label: lb.text

    width: explicitSize ? 200 : undefined; height: explicitSize ? 150 : undefined
    smooth: true

    Rectangle {
        border.color: "red"; color: "transparent"
        anchors.fill: parent
    }

    Text {
        id: lb
        anchors.top: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; anchors.topMargin: 4
    }
}
