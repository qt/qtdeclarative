// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    width: dial.implicitWidth + 20
    height: dial.implicitHeight + 20
    visible: true

    property alias dial: dial

    Dial {
        id: dial
        anchors.centerIn: parent
    }
}
