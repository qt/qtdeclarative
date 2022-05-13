// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    width: progressBar.implicitWidth
    height: 64
    visible: true

    property alias progressBar: progressBar

    ProgressBar {
        id: progressBar
        indeterminate: true
        anchors.centerIn: parent
    }
}
