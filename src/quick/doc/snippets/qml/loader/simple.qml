// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
//![0]
import QtQuick

Item {
    width: 200; height: 200

    Loader { id: pageLoader }

    MouseArea {
        anchors.fill: parent
        onClicked: pageLoader.source = "Page1.qml"
    }
}
//![0]
