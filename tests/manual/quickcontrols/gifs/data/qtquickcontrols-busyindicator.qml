// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    width: busyIndicator.implicitWidth
    height: busyIndicator.implicitHeight
    visible: true

    property alias busyIndicator: busyIndicator

    BusyIndicator {
        id: busyIndicator
        running: true
        anchors.centerIn: parent
    }
}
