// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12
import QtQuick.Window 2.2
import "qrc:/quick/shared/" as Examples

Window {
    width: 800
    height: 600
    visible: true
    Examples.LauncherList {
        id: ll
        objectName: "LauncherList"
        anchors.fill: parent
        Component.onCompleted: {
            addExample("plain mouse area", "Plain mouse area for testing flags passed from mouse event", Qt.resolvedUrl("plainMouseArea.qml"))
        }
    }
}
